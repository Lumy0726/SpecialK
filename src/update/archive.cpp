﻿/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/stdafx.h>

bool config_files_changed = false;

static ISzAlloc      g_Alloc  = { SzAlloc, SzFree };
static volatile LONG crc_init = 0;


void
SK_7Z_TableInit (void)
{
  if (! InterlockedCompareExchangeAcquire
                                 (&crc_init, 1, 0) )
  { CrcGenerateTable             (               );
    InterlockedIncrementRelease  (&crc_init      );
  } SK_Thread_SpinUntilAtomicMin (&crc_init, 2   );
}

void
SK_Get7ZFileContents (                 const wchar_t* wszArchive,
                       std::vector <sk_file_entry_s>& entries     )
{
  SK_7Z_TableInit ();

  CFileInStream arc_stream       = { };
  ISzAlloc      thread_alloc     = { };
  ISzAlloc      thread_tmp_alloc = { };

  std::unique_ptr    <CLookToRead> look_stream =
    std::make_unique <CLookToRead> ();

  FileInStream_CreateVTable (&arc_stream);
  LookToRead_CreateVTable   (look_stream.get (), False);

  look_stream->realStream = &arc_stream.s;
  LookToRead_Init          (look_stream.get ());

  thread_alloc.Alloc      = SzAlloc;
  thread_alloc.Free       = SzFree;

  thread_tmp_alloc.Alloc  = SzAllocTemp;
  thread_tmp_alloc.Free   = SzFreeTemp;

  if (SZ_OK != InFile_OpenW (&arc_stream.file, wszArchive))
  {
    return;
  }

  CSzArEx       arc = { };
  SzArEx_Init (&arc);

  if ( SzArEx_Open ( &arc,
                       &look_stream->s,
                         &thread_alloc,
                           &thread_tmp_alloc ) == SZ_OK )
  {
    wchar_t  wszEntry [MAX_PATH + 2] = { };

    for (uint32_t i = 0; i < arc.NumFiles; i++)
    {
      if (SzArEx_IsDir (&arc, i))
        continue;

      RtlSecureZeroMemory (wszEntry, (MAX_PATH + 2) * sizeof (wchar_t));
      SzArEx_GetFileNameUtf16 (&arc, i, (UInt16 *)wszEntry);

      uint64_t fileSize  = SzArEx_GetFileSize (&arc, i);
      wchar_t* pwszEntry = wszEntry;

      if (*wszEntry == L'\\')
        pwszEntry = CharNextW (pwszEntry);

      entries.emplace_back (
        sk_file_entry_s { i, fileSize, pwszEntry }
      );
    }

    File_Close  (&arc_stream.file);
  }

  SzArEx_Free (&arc, &thread_alloc);
}

#include <set>

HRESULT
SK_Decompress7z ( const wchar_t*            wszArchive,
                  const wchar_t*            wszOldVersion,
                  bool                      backup,
                  SK_7Z_DECOMP_PROGRESS_PFN callback )
{
  SK_7Z_TableInit ();

  // Don't back stuff up if we're installing :P
  if (SK_IsHostAppSKIM ())
    backup = false;

  std::vector <sk_file_entry_s>     all_files;
  SK_Get7ZFileContents (wszArchive, all_files);

  ///const size_t needed_size =
  ///  all_files.size ();

  std::vector <sk_file_entry_s> reg_files;
  std::vector <sk_file_entry_s> cfg_files;

  for ( auto& it : all_files )
  {
    if (wcsstr (it.name.c_str (), L"default_"))
    {
      cfg_files.emplace_back (it);
    }
    else
    {
      reg_files.emplace_back (it);
    }
  }

  CFileInStream arc_stream       = { };
  ISzAlloc      thread_alloc     = { };
  ISzAlloc      thread_tmp_alloc = { };

  std::unique_ptr    <CLookToRead> look_stream =
    std::make_unique <CLookToRead> ();

  FileInStream_CreateVTable (&arc_stream);
  LookToRead_CreateVTable   (look_stream.get (), False);

  look_stream->realStream = &arc_stream.s;
  LookToRead_Init          (look_stream.get ());

  thread_alloc.Alloc     = SzAlloc;
  thread_alloc.Free      = SzFree;

  thread_tmp_alloc.Alloc = SzAllocTemp;
  thread_tmp_alloc.Free  = SzFreeTemp;

  CSzArEx       arc = { };
  SzArEx_Init (&arc);

  uint32_t block_idx     = 0xFFFFFFFF;

  if ( InFile_OpenW (&arc_stream.file, wszArchive) ||
       SzArEx_Open ( &arc,
                       &look_stream->s,
                         &thread_alloc,
                           &thread_tmp_alloc ) != SZ_OK )
  {
    SK_LOG0 ( ( L" ** Cannot open archive file: %s",
                                       wszArchive ),
                L"7-Zip Arch" );

    SzArEx_Free (&arc, &thread_alloc);

    return E_FAIL;
  }

  size_t old_ver_len = 0;

  if (wszOldVersion != nullptr)
    StringCbLengthW (wszOldVersion, MAX_PATH + 2, &old_ver_len);

  int            i = 0;
  for ( auto& file : reg_files )
  {
    // Sanity Check
    if ( file.name.empty () ||
         file.filesize == 0  )
    {
      ++i;
      continue;
    }

    Byte*    out           = nullptr;
    size_t   out_len       = 0;
    size_t   offset        = 0;
    size_t   decomp_size   = 0;

    SK_LOG0 ( ( L"Extracting file ('%s')",
                     file.name.c_str () ),
                L"7-Zip Arch" );

    if ( SZ_OK !=
           SzArEx_Extract ( &arc,          &look_stream->s, file.fileno,
                            &block_idx,    &out,            &out_len,
                            &offset,       &decomp_size,
                            &thread_alloc, &thread_tmp_alloc ) )
    {
      SK_LOG0 ( ( L"Failed to extract 7-zip file ('%s')",
                     file.name.c_str () ),
                  L"7-Zip Arch" );
                  

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    wchar_t wszDestPath     [MAX_PATH + 2] = { };
    wchar_t wszMovePath     [MAX_PATH + 2] = { };

    wcsncpy_s ( wszDestPath, MAX_PATH,
                  SK_SYS_GetInstallPath ().c_str (), _TRUNCATE );
    lstrcatW  ( wszDestPath,     file.name.c_str () );

    if (GetFileAttributes (wszDestPath) != INVALID_FILE_ATTRIBUTES)
    {
      wcsncpy_s ( wszMovePath, MAX_PATH,
                    SK_SYS_GetVersionPath ().c_str (), _TRUNCATE );

      if ( wszOldVersion != nullptr &&
              old_ver_len           &&
                   backup )
      {
        lstrcatW (wszMovePath, wszOldVersion);
        lstrcatW (wszMovePath, L"\\");
      }

      lstrcatW (  wszMovePath, file.name.c_str ());

      // If the archive contains sub-directories, this will create them
      SK_CreateDirectories (wszMovePath);

      if ( wszOldVersion == nullptr ||
           (! old_ver_len)          ||
           (! backup)      )
      {
        lstrcatW (wszMovePath, L".old");
      }

      SK_File_MoveNoFail ( wszDestPath,
                            wszMovePath );
    }

    else
    {
      SK_CreateDirectories (wszDestPath);
    }

    HANDLE hOutFile =
      CreateFileW ( wszDestPath,
                      GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_SEQUENTIAL_SCAN,
                                nullptr );

    if (hOutFile != INVALID_HANDLE_VALUE)
    {
      DWORD dwWritten;

      WriteFile ( hOutFile,
                    out,
                      PtrToUint ((void *)out_len),
                        &dwWritten,
                            nullptr );

      SK_CloseHandle (hOutFile);

      // We still need to move files in order to replace ones currently
      //   involving execution, but if the user does not want backups,
      //     attempt to delete the moved files.
      if (! backup)
      {
        DeleteFileW (wszMovePath);
      }
    }

    else
    {
      SK_LOG0 ( ( L"Failed to open file : '%s'",
                     wszDestPath ),
                  L"7-Zip Arch" );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    if (callback != nullptr)
    {
      callback (++i, PtrToUint ((void *)reg_files.size ()));
    }
  }

  File_Close  (&arc_stream.file);


  block_idx     = 0xFFFFFFFF;

  if ( InFile_OpenW (&arc_stream.file, wszArchive) ||
       SzArEx_Open ( &arc,
                       &look_stream->s,
                         &thread_alloc,
                           &thread_tmp_alloc ) != SZ_OK )
  {
    SK_LOG0 ( ( L" ** Cannot open archive file: %s",
                     wszArchive ),
                L"7-Zip Arch" );

    SzArEx_Free (&arc, &thread_alloc);

    return E_FAIL;
  }

                     i = 0;
  for ( auto& cfg_file : cfg_files )
  {
    Byte*    out           = nullptr;
    size_t   out_len       = 0;
    size_t   offset        = 0;
    size_t   decomp_size   = 0;

    SK_LOG0 ( ( L"Extracting config file ('%s')",
                     cfg_file.name.c_str () ),
                L"7-Zip Arch" );

    if ( SZ_OK !=
           SzArEx_Extract ( &arc,          &look_stream->s, cfg_file.fileno,
                            &block_idx,    &out,            &out_len,
                            &offset,       &decomp_size,
                            &thread_alloc, &thread_tmp_alloc ) )
    {
      SK_LOG0 ( ( L"Failed to extract 7-zip config file ('%s')",
                     cfg_file.name.c_str () ),
                  L"7-Zip Arch" );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    wchar_t wszDefaultConfig [MAX_PATH + 2] = { }; // Existing Default Cfg
    wchar_t wszNewConfig     [MAX_PATH + 2] = { }; // Just Downloaded

    wchar_t wszUserConfig    [MAX_PATH + 2] = { }; // Currently Deployed
    wchar_t wszOldConfig     [MAX_PATH + 2] = { }; // Backed-Up User Cfg

    wcsncpy_s ( wszDefaultConfig,                   MAX_PATH,
                SK_SYS_GetInstallPath ().c_str (), _TRUNCATE );
    wcsncpy_s ( wszUserConfig,                      MAX_PATH,
                SK_SYS_GetInstallPath ().c_str (), _TRUNCATE );

    PathAppend (wszDefaultConfig, cfg_file.name.c_str ());
    PathAppend (wszUserConfig,    cfg_file.name.c_str ());

    wchar_t* wszDefault_ =
      wcsstr (wszUserConfig, L"default_");

    if (wszDefault_ != nullptr)
    {
      *wszDefault_ = L'\0';

      wchar_t* wsz_ =
        wcschr ( wszDefault_+1, L'_' );

      if (wsz_ != nullptr)
      {
        lstrcatW (wszUserConfig, wsz_+1);
      }
    }

    lstrcatW ( wszOldConfig, wszUserConfig );
    lstrcatW ( wszOldConfig, L".old"       );

    lstrcatW (wszNewConfig, wszDefaultConfig);
    lstrcatW (wszNewConfig, L".new");

    SK_CreateDirectories (wszNewConfig);

    HANDLE hOutFile =
      CreateFileW ( wszNewConfig,
                      GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                          nullptr,
                            CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_SEQUENTIAL_SCAN,
                                nullptr );

    if (hOutFile != INVALID_HANDLE_VALUE)
    {
      DWORD dwWritten;

      WriteFile ( hOutFile,
                    out,
                      PtrToUint ((void *)out_len),
                        &dwWritten,
                            nullptr );

      SK_CloseHandle (hOutFile);

      if ( GetFileAttributes (wszUserConfig)     == INVALID_FILE_ATTRIBUTES ||
           GetFileAttributes (wszDefaultConfig)  == INVALID_FILE_ATTRIBUTES ||
             SK_File_GetCRC32C (wszDefaultConfig, nullptr) !=
             SK_File_GetCRC32C (wszNewConfig,     nullptr) )
      {
        if (GetFileAttributes (wszUserConfig) != INVALID_FILE_ATTRIBUTES)
          config_files_changed = true;

        SK_File_MoveNoFail ( wszNewConfig,
                              wszDefaultConfig );

        SK_File_MoveNoFail ( wszUserConfig,
                              wszOldConfig );

        CopyFileW   ( wszDefaultConfig,
                        wszUserConfig,
                          FALSE );
      }

      else
      {
        DeleteFileW (wszNewConfig);
      }
    }

    else
    {
      SK_LOG0 ( ( L"Failed to create file: '%s'",
                     wszNewConfig ),
                  L"7-Zip Arch" );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    if (callback != nullptr)
    {
      callback (++i, PtrToUint ((void *)cfg_files.size ()));
    }
  }

  File_Close  (&arc_stream.file);
  SzArEx_Free (&arc, &thread_alloc);

  return S_OK;
}


HRESULT
SK_Decompress7zEx ( const wchar_t*            wszArchive,
                    const wchar_t*            wszDestination,
                    SK_7Z_DECOMP_PROGRESS_PFN callback )
{
  SK_7Z_TableInit ();

  std::vector <sk_file_entry_s>     files;
  SK_Get7ZFileContents (wszArchive, files);

  CFileInStream arc_stream       = { };
  ISzAlloc      thread_alloc     = { };
  ISzAlloc      thread_tmp_alloc = { };

  std::unique_ptr    <CLookToRead> look_stream =
    std::make_unique <CLookToRead> ();

  FileInStream_CreateVTable (&arc_stream);
  LookToRead_CreateVTable   (look_stream.get (), True);

  look_stream->realStream = &arc_stream.s;
  LookToRead_Init          (look_stream.get ());

  thread_alloc.Alloc     = SzAlloc;
  thread_alloc.Free      = SzFree;

  thread_tmp_alloc.Alloc = SzAllocTemp;
  thread_tmp_alloc.Free  = SzFreeTemp;

  CSzArEx       arc = { };
  SzArEx_Init (&arc);

  uint32_t block_idx     = 0xFFFFFFFF;

  if ( InFile_OpenW (&arc_stream.file, wszArchive) ||
       SzArEx_Open ( &arc,
                       &look_stream->s,
                         &thread_alloc,
                           &thread_tmp_alloc ) != SZ_OK )
  {
    SK_LOG0 ( ( L" ** Cannot open archive file: %s",
                     wszArchive ),
                L"7ZIP-Unpak" );

    SzArEx_Free (&arc, &thread_alloc);

    return E_FAIL;
  }

  for (unsigned int i = 0; i < files.size (); i++)
  {
    auto& file =
      files [i];

    Byte*    out           = nullptr;
    size_t   out_len       = 0;
    size_t   offset        = 0;
    size_t   decomp_size   = 0;

    SK_LOG0 ( ( L"Extracting file ('%s')",
                files [i].name.c_str () ),
                L"7ZIP-Unpak" );

    if ( SZ_OK !=
           SzArEx_Extract ( &arc,          &look_stream->s, file.fileno,
                            &block_idx,    &out,            &out_len,
                            &offset,       &decomp_size,
                            &thread_alloc, &thread_tmp_alloc ) )
    {
      SK_LOG0 ( ( L"Failed to extract 7-zip file ('%s')",
                                    file.name.c_str () ),
                  L"7ZIP-Unpak" );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    wchar_t       wszDestPath     [MAX_PATH + 2] = { };
    wcsncpy_s   ( wszDestPath,     MAX_PATH,
                  wszDestination, _TRUNCATE) ;

    PathAppendW          (wszDestPath, file.name.c_str ());
    SK_CreateDirectories (wszDestPath);

    HANDLE hOutFile =
      CreateFileW ( wszDestPath,
                      GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          nullptr,
                            CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL |
                              FILE_FLAG_SEQUENTIAL_SCAN,
                                nullptr );

    if (hOutFile != INVALID_HANDLE_VALUE)
    {
      DWORD dwWritten;

      WriteFile ( hOutFile,
                    out,
                      PtrToUint ((void *)out_len),
                        &dwWritten,
                            nullptr );

      SK_CloseHandle (hOutFile);
    }

    else
    {
      SK_LOG0 ( ( L"Failed to open file: '%s'",
                                 wszDestPath ),
                  L"7ZIP-Unpak" );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_FAIL;
    }

    if (callback != nullptr)
    {
      callback (i + 1, PtrToUint ((void *)files.size ()));
    }
  }

  File_Close  (&arc_stream.file);

  return S_OK;
}
