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

#ifndef __SK__LOAD_LIBRARY_H__
#define __SK__LOAD_LIBRARY_H__

void __stdcall SK_InitCompatBlacklist (void);
void __stdcall SK_PreInitLoadLibrary  (void) ; // Before we hook load library, just import it

bool __stdcall SK_ReHookLoadLibrary   (void);
void __stdcall SK_UnhookLoadLibrary   (void);

void __stdcall SK_LockDllLoader       (void);
void __stdcall SK_UnlockDllLoader     (void);

#ifdef _INC_WINDOWS

typedef BOOL    (WINAPI *FreeLibrary_pfn)  (HMODULE hLibModule);
typedef HMODULE (WINAPI *LoadLibraryA_pfn) (LPCSTR  lpFileName);
typedef HMODULE (WINAPI *LoadLibraryW_pfn) (LPCWSTR lpFileName);
typedef HMODULE (WINAPI *LoadLibraryExA_pfn)
( _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);
typedef HMODULE (WINAPI *LoadLibraryExW_pfn)
( _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

typedef HMODULE (WINAPI *LoadPackagedLibrary_pfn)
( _In_       LPCWSTR lpwLibFileName,
  _Reserved_ DWORD   Reserved
);

extern FreeLibrary_pfn         FreeLibrary_Original;

extern LoadLibraryA_pfn        LoadLibraryA_Original;
extern LoadLibraryW_pfn        LoadLibraryW_Original;

extern LoadLibraryExA_pfn      LoadLibraryExA_Original;
extern LoadLibraryExW_pfn      LoadLibraryExW_Original;

extern LoadPackagedLibrary_pfn LoadPackagedLibrary_Original;

BOOL __stdcall SK_FreeLibrary (HMODULE hMod);
HMODULE WINAPI SK_LoadLibraryA (LPCSTR  lpFileName);
HMODULE WINAPI SK_LoadLibraryW (LPCWSTR lpFileName);
HMODULE WINAPI SK_LoadLibraryExA (
  _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags );
HMODULE WINAPI SK_LoadLibraryExW (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags );

template <typename _T>
BOOL
__stdcall
SK_LoadLibrary_PinModule (const _T* pStr);

struct iSK_Logger;

void
SK_PrintUnloadedDLLs (iSK_Logger* pLogger);
#endif
#endif /* __SK__LOAD_LIBRARY_H__ */
