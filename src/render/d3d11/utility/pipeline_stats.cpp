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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_screenshot.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>

void
__stdcall
SK_D3D11_UpdateRenderStatsEx (const IDXGISwapChain* pSwapChain)
{
  if (pSwapChain == nullptr)
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto pDev =
    rb.getDevice <ID3D11Device> ();
  SK_ComQIPtr    <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if (! ( pDev         != nullptr &&
          pDevCtx      != nullptr &&
          rb.swapchain != nullptr ) )
    return;

  SK::DXGI::PipelineStatsD3D11& pipeline_stats =
    SK::DXGI::pipeline_stats_d3d11;

  if (ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async) != nullptr)
  {
    if (ReadAcquire (&pipeline_stats.query.active) == TRUE)
    {
      pDevCtx->End ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async));
      InterlockedExchange (&pipeline_stats.query.active, FALSE);
    }

    else
    {
      const HRESULT hr =
        pDevCtx->GetData ( (ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async),
                            &pipeline_stats.last_results,
                              sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS,
                                0x0 );
      if (hr == S_OK)
      {
        ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async))->Release ();
                       InterlockedExchangePointer ((void **)         &pipeline_stats.query.async, nullptr);
      }
    }
  }

  else
  {
    D3D11_QUERY_DESC query_desc
      {  D3D11_QUERY_PIPELINE_STATISTICS,
         0x00                             };

    ID3D11Query* pQuery = nullptr;

    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&pipeline_stats.query.async,  pQuery);
      pDevCtx->Begin             (                                       pQuery);
      InterlockedExchange        (         &pipeline_stats.query.active, TRUE);
    }
  }
}

void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && ( config.render.show ||
                          ( SK_ImGui_Widgets->d3d11_pipeline != nullptr &&
                          ( SK_ImGui_Widgets->d3d11_pipeline->isActive () ) ) ) ) )
    return;

  SK_D3D11_UpdateRenderStatsEx (pSwapChain);
}

std::string
SK_CountToString (uint64_t count)
{
  char str [64] = { };

  unsigned int unit = 0;

  if      (count > 1000000000UL) unit = 1000000000UL;
  else if (count > 1000000)      unit = 1000000UL;
  else if (count > 1000)         unit = 1000UL;
  else                           unit = 1UL;

  switch (unit)
  {
    case 1000000000UL:
      sprintf (str, "%6.2f Billion ", (float)count / (float)unit);
      break;
    case 1000000UL:
      sprintf (str, "%6.2f Million ", (float)count / (float)unit);
      break;
    case 1000UL:
      sprintf (str, "%6.2f Thousand", (float)count / (float)unit);
      break;
    case 1UL:
    default:
      sprintf (str, "%15llu", count);
      break;
  }

  return str;
}

void
SK_D3D11_SetPipelineStats (void* pData) noexcept
{
  memcpy ( &SK::DXGI::pipeline_stats_d3d11.last_results,
             pData,
               sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS));
}

void
SK_D3D11_GetVertexPipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.VSInvocations > 0)
  {
    sprintf ( szDesc,
                "  VERTEX : %s   (%s Verts ==> %s Triangles)",
                  SK_CountToString (stats.VSInvocations).c_str (),
                    SK_CountToString (stats.IAVertices).c_str (),
                      SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                "  VERTEX : <Unused>" );
  }
}

void
SK_D3D11_GetGeometryPipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.GSInvocations > 0)
  {
    sprintf ( szDesc,
                "%s  GEOM   : %s   (%s Prims)",
                  std::string (szDesc).c_str (),
                    SK_CountToString (stats.GSInvocations).c_str (),
                      SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                "%s  GEOM   : <Unused>",
                  std::string (szDesc).c_str () );
  }
}

void
SK_D3D11_GetTessellationPipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    sprintf ( szDesc,
                "%s  TESS   : %s Hull ==> %s Domain",
                  std::string (szDesc).c_str (),
                    SK_CountToString (stats.HSInvocations).c_str (),
                      SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    sprintf ( szDesc,
                "%s  TESS   : <Unused>",
                  std::string (szDesc).c_str () );
  }
}

void
SK_D3D11_GetRasterPipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CInvocations > 0)
  {
    sprintf ( szDesc,
                "%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
                  std::string (szDesc).c_str (), 100.0 *
                      ( static_cast <double> (stats.CPrimitives) /
                        static_cast <double> (stats.CInvocations) ),
                    SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                "%s  RASTER : <Unused>",
                  std::string (szDesc).c_str () );
  }
}

void
SK_D3D11_GetPixelPipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.PSInvocations > 0)
  {
    sprintf ( szDesc,
                "%s  PIXEL  : %s   (%s Triangles OUT)",
                  std::string (szDesc).c_str (),
                    SK_CountToString (stats.PSInvocations).c_str (),
                      SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                "%s  PIXEL  : <Unused>",
                  std::string (szDesc).c_str () );
  }
}

void
SK_D3D11_GetComputePipelineDesc (char* szDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CSInvocations > 0)
  {
    sprintf ( szDesc,
                "%s  COMPUTE: %s",
                  std::string (szDesc).c_str (),
  SK_CountToString (stats.CSInvocations).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                "%s  COMPUTE: <Unused>",
                  std::string (szDesc).c_str () );
  }
}

std::string
SK::DXGI::getPipelineStatsDesc (void)
{
  char szDesc [1024] = { };

  //
  // VERTEX SHADING
  //
  SK_D3D11_GetVertexPipelineDesc (szDesc);       lstrcatA (szDesc, "\n");

  //
  // GEOMETRY SHADING
  //
  SK_D3D11_GetGeometryPipelineDesc (szDesc);     lstrcatA (szDesc, "\n");

  //
  // TESSELLATION
  //
  SK_D3D11_GetTessellationPipelineDesc (szDesc); lstrcatA (szDesc, "\n");

  //
  // RASTERIZATION
  //
  SK_D3D11_GetRasterPipelineDesc (szDesc);       lstrcatA (szDesc, "\n");

  //
  // PIXEL SHADING
  //
  SK_D3D11_GetPixelPipelineDesc (szDesc);        lstrcatA (szDesc, "\n");

  //
  // COMPUTE
  //
  SK_D3D11_GetComputePipelineDesc (szDesc);      lstrcatA (szDesc, "\n");

  return szDesc;
}
