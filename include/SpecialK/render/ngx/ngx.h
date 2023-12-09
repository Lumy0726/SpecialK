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

#pragma once

#include <SpecialK/thread.h>
#include <SpecialK/utility/lazy_global.h>

#include <render/ngx/ngx_defs.h>

struct NGX_ThreadSafety {
  struct {
    SK_Thread_HybridSpinlock Params;
  } locks;
};

extern SK_LazyGlobal <NGX_ThreadSafety> SK_NGX_Threading;

void SK_NGX_EstablishDLSSVersion  (void) noexcept;
void SK_NGX_EstablishDLSSGVersion (void) noexcept;

struct SK_DLSS_Context
{
  // Has the DLSS context for this API (i.e. D3D11, D3D12, CUDA)
  //   made any API calls?
  bool apis_called                  = false;

  struct version_s {
    unsigned int major, minor, build, revision;
  };

  struct dlss_s {
    NVSDK_NGX_Handle*    Handle         = nullptr;
    NVSDK_NGX_Parameter* Parameters     = nullptr;
    volatile ULONG64     LastFrame      = 0ULL;
    volatile ULONG64     ResetFrame     = 0ULL; // If >= Current Frame, issue a DLSS Reset
    NVSDK_NGX_Feature    DLSS_Type      = NVSDK_NGX_Feature_SuperSampling;
    static DWORD         IndicatorFlags;
    static version_s     Version;

    static bool hasSharpening       (void) { return ( Version.major <= 2 && ( Version.major != 2 ||   Version.minor < 5 ||   Version.build < 1 ) );                             };
    static bool hasDLAAQualityLevel (void) { return ( Version.major  > 3 || ( Version.major == 3 && ( Version.minor > 1 || ( Version.minor == 1 && Version.build >= 13 ) ) ) ); };

    static void showIndicator    (bool show);
    static bool isIndicatorShown (void);
  } super_sampling;

  struct dlssg_s {
    NVSDK_NGX_Handle*    Handle         = nullptr;
    NVSDK_NGX_Parameter* Parameters     = nullptr;
    volatile ULONG64     LastFrame      = 0ULL;
    static DWORD         IndicatorFlags;
    static version_s     Version;

    static void showIndicator    (bool show);
    static bool isIndicatorShown (void);
  } frame_gen;

  inline void log_call (void) noexcept { apis_called = true; SK_NGX_EstablishDLSSVersion (); };
};

extern SK_DLSS_Context SK_NGX_DLSS12;
extern SK_DLSS_Context SK_NGX_DLSS11;

void             NVSDK_CONV NVSDK_NGX_Parameter_SetI_Detour           (      NVSDK_NGX_Parameter* InParameter, const char* InName, int            InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetUI_Detour          (      NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int   InValue);
void             NVSDK_CONV NVSDK_NGX_Parameter_SetF_Detour           (      NVSDK_NGX_Parameter* InParameter, const char* InName, float          InValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetVoidPointer_Detour (const NVSDK_NGX_Parameter* InParameter, const char* InName, void        **OutValue);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_Parameter_GetUI_Detour          (const NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int *OutValue);

using NVSDK_NGX_Parameter_SetF_pfn           = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName,          float InValue);
using NVSDK_NGX_Parameter_SetI_pfn           = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName,          int   InValue);
using NVSDK_NGX_Parameter_SetUI_pfn          = void             (NVSDK_CONV *)(      NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int   InValue);
using NVSDK_NGX_Parameter_GetUI_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int* OutValue);
using NVSDK_NGX_Parameter_GetVoidPointer_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(const NVSDK_NGX_Parameter *InParameter, const char* InName, void**        OutValue);

extern NVSDK_NGX_Parameter_SetF_pfn           NVSDK_NGX_Parameter_SetF_Original;
extern NVSDK_NGX_Parameter_SetI_pfn           NVSDK_NGX_Parameter_SetI_Original;
extern NVSDK_NGX_Parameter_SetUI_pfn          NVSDK_NGX_Parameter_SetUI_Original;
extern NVSDK_NGX_Parameter_GetUI_pfn          NVSDK_NGX_Parameter_GetUI_Original;
extern NVSDK_NGX_Parameter_GetVoidPointer_pfn NVSDK_NGX_Parameter_GetVoidPointer_Original;

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
using  GetNGXResultAsString_pfn = const wchar_t* (NVSDK_CONV *)(NVSDK_NGX_Result InNGXResult);
extern GetNGXResultAsString_pfn GetNGXResultAsString;

bool SK_NGX_HookParameters (NVSDK_NGX_Parameter *Params);
void SK_NGX_Reset          (void);

void SK_NGX_DumpParameters (const NVSDK_NGX_Parameter *Params);

void
NVSDK_CONV
SK_NGX_LogCallback ( const char*             message,
                     NVSDK_NGX_Logging_Level loggingLevel,
                     NVSDK_NGX_Feature       sourceComponent );

const char*
SK_NGX_FeatureToStr (NVSDK_NGX_Feature feature) noexcept;
