﻿// dear imgui: Renderer for DirectX12
// This needs to be used along with a Platform Binding (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'D3D12_GPU_DESCRIPTOR_HANDLE' as ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.
// Issues:
//  [ ] 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*)). See github.com/ocornut/imgui/pull/301

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

enum DXGI_FORMAT;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

#include <imgui/imgui.h>

// cmd_list is the command list that the implementation will use to render imgui draw lists.
// Before calling the render function, caller must prepare cmd_list by resetting it and setting the appropriate 
// render target and descriptor heap that contains font_srv_cpu_desc_handle/font_srv_gpu_desc_handle.
// font_srv_cpu_desc_handle and font_srv_gpu_desc_handle are handles to a single SRV descriptor to use for the internal font texture.
bool     ImGui_ImplDX12_Init ( ID3D12Device*               device,     int                         num_frames_in_flight,
                               DXGI_FORMAT                 rtv_format, D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle,
                               D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle );

void     ImGui_ImplDX12_Shutdown       (void);
void     ImGui_ImplDX12_NewFrame       (void);
void     ImGui_ImplDX12_RenderDrawData ( ImDrawData*                draw_data,
                                         ID3D12GraphicsCommandList* graphics_command_list );

// Use if you want to reset your rendering device without losing ImGui state.
void     ImGui_ImplDX12_InvalidateDeviceObjects (void);
bool     ImGui_ImplDX12_CreateDeviceObjects     (void);

struct sk_d3d12_texture_s {
  ID3D12Resource             *pTexture                 = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE hTextureSrvCpuDescHandle = { };
  D3D12_GPU_DESCRIPTOR_HANDLE hTextureSrvGpuDescHandle = { };

  static int num_textures;
};

sk_d3d12_texture_s
SK_D3D12_CreateDXTex ( DirectX::TexMetadata&  metadata,
                       DirectX::ScratchImage& image );
