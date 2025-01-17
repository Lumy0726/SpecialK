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

#include <imgui/imgui.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <imgui/backends/imgui_vk.h>

// Data
static INT64        g_Time                   = 0;
static INT64        g_TicksPerSecond         = 0;

// Vulkan Data
static VkAllocationCallbacks* g_Allocator      = NULL;
static VkPhysicalDevice       g_Gpu            = VK_NULL_HANDLE;
static VkDevice               g_Device         = VK_NULL_HANDLE;
static VkRenderPass           g_RenderPass     = VK_NULL_HANDLE;
static VkPipelineCache        g_PipelineCache  = VK_NULL_HANDLE;
static VkDescriptorPool       g_DescriptorPool = VK_NULL_HANDLE;
static void (*g_CheckVkResult)(VkResult err)   = NULL;

static VkCommandBuffer        g_CommandBuffer         = VK_NULL_HANDLE;
static VkDeviceSize           g_BufferMemoryAlignment = 256ULL;
static VkPipelineCreateFlags  g_PipelineCreateFlags   = 0;
static int                    g_FrameIndex            = 0;

static VkDescriptorSetLayout  g_DescriptorSetLayout = VK_NULL_HANDLE;
static VkPipelineLayout       g_PipelineLayout      = VK_NULL_HANDLE;
static VkDescriptorSet        g_DescriptorSet       = VK_NULL_HANDLE;
static VkPipeline             g_Pipeline            = VK_NULL_HANDLE;

static VkSampler              g_FontSampler = VK_NULL_HANDLE;
static VkDeviceMemory         g_FontMemory  = VK_NULL_HANDLE;
static VkImage                g_FontImage   = VK_NULL_HANDLE;
static VkImageView            g_FontView    = VK_NULL_HANDLE;

static VkDeviceMemory         g_VertexBufferMemory [IMGUI_VK_QUEUED_FRAMES] = { };
static VkDeviceMemory         g_IndexBufferMemory  [IMGUI_VK_QUEUED_FRAMES] = { };
static VkDeviceSize           g_VertexBufferSize   [IMGUI_VK_QUEUED_FRAMES] = { };
static VkDeviceSize           g_IndexBufferSize    [IMGUI_VK_QUEUED_FRAMES] = { };
static VkBuffer               g_VertexBuffer       [IMGUI_VK_QUEUED_FRAMES] = { };
static VkBuffer               g_IndexBuffer        [IMGUI_VK_QUEUED_FRAMES] = { };

static VkDeviceMemory         g_UploadBufferMemory = VK_NULL_HANDLE;
static VkBuffer               g_UploadBuffer       = VK_NULL_HANDLE;

static uint32_t __glsl_shader_vert_spv [] =
{
  0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
  0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
  0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
  0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
  0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
  0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
  0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
  0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
  0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
  0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
  0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
  0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
  0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
  0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
  0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
  0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
  0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
  0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
  0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
  0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
  0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
  0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
  0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
  0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
  0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
  0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
  0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
  0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
  0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
  0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
  0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
  0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
  0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
  0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
  0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
  0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
  0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
  0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
  0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
  0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
  0x0000002d,0x0000002c,0x000100fd,0x00010038
};

static uint32_t __glsl_shader_frag_spv [] =
{
  0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
  0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
  0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
  0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
  0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
  0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
  0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
  0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
  0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
  0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
  0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
  0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
  0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
  0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
  0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
  0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
  0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
  0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
  0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
  0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
  0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
  0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
  0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
  0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
  0x00010038
};

static PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
static PFN_vkAllocateMemory                    vkAllocateMemory;
static PFN_vkFreeMemory                        vkFreeMemory;
static PFN_vkMapMemory                         vkMapMemory;
static PFN_vkUnmapMemory                       vkUnmapMemory;
static PFN_vkFlushMappedMemoryRanges           vkFlushMappedMemoryRanges;
static PFN_vkBindBufferMemory                  vkBindBufferMemory;
static PFN_vkBindImageMemory                   vkBindImageMemory;
static PFN_vkGetBufferMemoryRequirements       vkGetBufferMemoryRequirements;
static PFN_vkGetImageMemoryRequirements        vkGetImageMemoryRequirements;
static PFN_vkCreateBuffer                      vkCreateBuffer;
static PFN_vkDestroyBuffer                     vkDestroyBuffer;
static PFN_vkCreateImage                       vkCreateImage;
static PFN_vkDestroyImage                      vkDestroyImage;
static PFN_vkCreateImageView                   vkCreateImageView;
static PFN_vkDestroyImageView                  vkDestroyImageView;
static PFN_vkCreateShaderModule                vkCreateShaderModule;
static PFN_vkDestroyShaderModule               vkDestroyShaderModule;
static PFN_vkCreateGraphicsPipelines           vkCreateGraphicsPipelines;
static PFN_vkDestroyPipeline                   vkDestroyPipeline;
static PFN_vkCreatePipelineLayout              vkCreatePipelineLayout;
static PFN_vkDestroyPipelineLayout             vkDestroyPipelineLayout;
static PFN_vkCreateSampler                     vkCreateSampler;
static PFN_vkDestroySampler                    vkDestroySampler;
static PFN_vkCreateDescriptorSetLayout         vkCreateDescriptorSetLayout;
static PFN_vkDestroyDescriptorSetLayout        vkDestroyDescriptorSetLayout;
static PFN_vkAllocateDescriptorSets            vkAllocateDescriptorSets;
static PFN_vkUpdateDescriptorSets              vkUpdateDescriptorSets;
static PFN_vkCmdBindPipeline                   vkCmdBindPipeline;
static PFN_vkCmdSetViewport                    vkCmdSetViewport;
static PFN_vkCmdSetScissor                     vkCmdSetScissor;
static PFN_vkCmdBindDescriptorSets             vkCmdBindDescriptorSets;
static PFN_vkCmdBindIndexBuffer                vkCmdBindIndexBuffer;
static PFN_vkCmdBindVertexBuffers              vkCmdBindVertexBuffers;
static PFN_vkCmdDrawIndexed                    vkCmdDrawIndexed;
static PFN_vkCmdCopyBufferToImage              vkCmdCopyBufferToImage;
static PFN_vkCmdPipelineBarrier                vkCmdPipelineBarrier;
static PFN_vkCmdPushConstants                  vkCmdPushConstants;


static
uint32_t
ImGui_ImplVulkan_MemoryType (VkMemoryPropertyFlags properties, uint32_t type_bits)
{
  VkPhysicalDeviceMemoryProperties prop;
  vkGetPhysicalDeviceMemoryProperties (g_Gpu, &prop);

  for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
    if ((prop.memoryTypes [i].propertyFlags & properties) == properties && type_bits & (1 << i))
      return i;

  return 0xffffffff; // Unable to find memoryType
}

static
void
ImGui_ImplVulkan_VkResult (VkResult err)
{
  if (g_CheckVkResult)
    g_CheckVkResult (err);
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
void
ImGui_ImplVulkan_RenderDrawLists (ImDrawData* draw_data)
{
  VkResult err { };

  ImGuiIO& io =
    ImGui::GetIO ();

  // Create the Vertex Buffer:
  VkDeviceSize vertex_size =
    draw_data->TotalVtxCount * sizeof (ImDrawVert);

  if (! g_VertexBuffer [g_FrameIndex] || g_VertexBufferSize[g_FrameIndex] < vertex_size)
  {
    if (g_VertexBuffer [g_FrameIndex])
      vkDestroyBuffer (g_Device, g_VertexBuffer [g_FrameIndex], g_Allocator);

    if (g_VertexBufferMemory [g_FrameIndex])
      vkFreeMemory (g_Device, g_VertexBufferMemory [g_FrameIndex], g_Allocator);

    VkDeviceSize vertex_buffer_size =
      ((vertex_size - 1) / g_BufferMemoryAlignment + 1) * g_BufferMemoryAlignment;

    VkBufferCreateInfo buffer_info = { };

    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = vertex_buffer_size;
    buffer_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err =
      vkCreateBuffer (g_Device, &buffer_info, g_Allocator, &g_VertexBuffer [g_FrameIndex]);

    ImGui_ImplVulkan_VkResult (err);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements (g_Device, g_VertexBuffer [g_FrameIndex], &req);

    g_BufferMemoryAlignment =
      (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment :
                                                  req.alignment;

    VkMemoryAllocateInfo alloc_info = { };

    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

    err =
      vkAllocateMemory (g_Device, &alloc_info, g_Allocator, &g_VertexBufferMemory [g_FrameIndex]);

    ImGui_ImplVulkan_VkResult (err);

    err =
      vkBindBufferMemory (g_Device, g_VertexBuffer [g_FrameIndex], g_VertexBufferMemory [g_FrameIndex], 0);

    ImGui_ImplVulkan_VkResult (err);

    g_VertexBufferSize [g_FrameIndex] = vertex_buffer_size;
  }

  // Create the Index Buffer: 
  VkDeviceSize index_size = draw_data->TotalIdxCount * sizeof (ImDrawIdx);

  if (! g_IndexBuffer [g_FrameIndex] || g_IndexBufferSize [g_FrameIndex] < index_size)
  {
    if (g_IndexBuffer [g_FrameIndex])
      vkDestroyBuffer (g_Device, g_IndexBuffer [g_FrameIndex], g_Allocator);

    if (g_IndexBufferMemory [g_FrameIndex])
      vkFreeMemory (g_Device, g_IndexBufferMemory [g_FrameIndex], g_Allocator);

    VkDeviceSize index_buffer_size =
      ((index_size - 1) / g_BufferMemoryAlignment + 1) * g_BufferMemoryAlignment;

    VkBufferCreateInfo buffer_info = { };

    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = index_buffer_size;
    buffer_info.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err =
      vkCreateBuffer (g_Device, &buffer_info, g_Allocator, &g_IndexBuffer [g_FrameIndex]);

    ImGui_ImplVulkan_VkResult (err);

    VkMemoryRequirements req;

    vkGetBufferMemoryRequirements (g_Device, g_IndexBuffer [g_FrameIndex], &req);

    g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment :
                                                                          req.alignment;
    VkMemoryAllocateInfo alloc_info = { };

    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

    err =
      vkAllocateMemory (g_Device, &alloc_info, g_Allocator, &g_IndexBufferMemory [g_FrameIndex]);

    ImGui_ImplVulkan_VkResult (err);

    err =
      vkBindBufferMemory (g_Device, g_IndexBuffer [g_FrameIndex], g_IndexBufferMemory [g_FrameIndex], 0);

    ImGui_ImplVulkan_VkResult (err);

    g_IndexBufferSize [g_FrameIndex] = index_buffer_size;
  }

  // Upload Vertex and index Data:
  {
    ImDrawVert* vtx_dst = 0;
    ImDrawIdx*  idx_dst = 0;

    err =
      vkMapMemory (g_Device, g_VertexBufferMemory [g_FrameIndex], 0, vertex_size, 0, (void**)(&vtx_dst));

    ImGui_ImplVulkan_VkResult (err);

    err =
      vkMapMemory (g_Device, g_IndexBufferMemory [g_FrameIndex], 0, index_size, 0, (void**)(&idx_dst));

    ImGui_ImplVulkan_VkResult (err);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
      const ImDrawList* cmd_list =
        draw_data->CmdLists [n];

      memcpy (vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof (ImDrawVert));
      memcpy (idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx));

      vtx_dst += cmd_list->VtxBuffer.Size;
      idx_dst += cmd_list->IdxBuffer.Size;
    }

    VkMappedMemoryRange range [2] = { };

    range [0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range [0].memory = g_VertexBufferMemory [g_FrameIndex];
    range [0].size   = vertex_size;
    range [1].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range [1].memory = g_IndexBufferMemory [g_FrameIndex];
    range [1].size   = index_size;

    err =
      vkFlushMappedMemoryRanges (g_Device, 2, range);

    ImGui_ImplVulkan_VkResult (err);

    vkUnmapMemory (g_Device, g_VertexBufferMemory [g_FrameIndex]);
    vkUnmapMemory (g_Device, g_IndexBufferMemory  [g_FrameIndex]);
  }

  // Bind pipeline and descriptor sets:
  {
    vkCmdBindPipeline (g_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipeline);

    VkDescriptorSet desc_set [1] = { g_DescriptorSet };

    vkCmdBindDescriptorSets (g_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);
  }

  // Bind Vertex And Index Buffer:
  {
    VkBuffer     vertex_buffers [1] = { g_VertexBuffer [g_FrameIndex] };
    VkDeviceSize vertex_offset  [1] = { 0 };

    vkCmdBindVertexBuffers (g_CommandBuffer, 0, 1, vertex_buffers, vertex_offset);
    vkCmdBindIndexBuffer   (g_CommandBuffer, g_IndexBuffer [g_FrameIndex], 0, VK_INDEX_TYPE_UINT16);
  }

  // Setup viewport:
  {
    VkViewport viewport;

    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = ImGui::GetIO ().DisplaySize.x;
    viewport.height   = ImGui::GetIO ().DisplaySize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport (g_CommandBuffer, 0, 1, &viewport);
  }

  // Setup scale and translation:
  {
    float scale [2];
    scale       [0] = 2.0f / io.DisplaySize.x;
    scale       [1] = 2.0f / io.DisplaySize.y;

    float translate [2];

    translate [0] = -1.0f;
    translate [1] = -1.0f;

    vkCmdPushConstants (g_CommandBuffer, g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
    vkCmdPushConstants (g_CommandBuffer, g_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
  }

  // Render the command lists:
  int vtx_offset = 0;
  int idx_offset = 0;

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists [n];

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd =
        &cmd_list->CmdBuffer [cmd_i];

      if (pcmd->UserCallback)
      {
        pcmd->UserCallback (cmd_list, pcmd);
      }

      else
      {
        VkRect2D scissor;

        scissor.offset.x      = (int32_t)(pcmd->ClipRect.x);
        scissor.offset.y      = (int32_t)(pcmd->ClipRect.y);
        scissor.extent.width  = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
        scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // TODO: + 1??????

        vkCmdSetScissor  (g_CommandBuffer, 0, 1, &scissor);
        vkCmdDrawIndexed (g_CommandBuffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
      }

      idx_offset += pcmd->ElemCount;
    }

    vtx_offset += cmd_list->VtxBuffer.Size;
  }
}

bool
ImGui_ImplVulkan_CreateFontsTexture (VkCommandBuffer command_buffer)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  SK_ImGui_LoadFonts ();

  unsigned char* pixels = nullptr;
  int            width  = 0,
                 height = 0;

  io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

  size_t upload_size =
    width * height * 4 * sizeof (uint8_t);

  VkResult err = VK_NOT_READY;

  // Create the Image:
  {
    VkImageCreateInfo info = { };

    info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType     = VK_IMAGE_TYPE_2D;
    info.format        = VK_FORMAT_R8G8B8A8_UNORM;
    info.extent.width  = width;
    info.extent.height = height;
    info.extent.depth  = 1;
    info.mipLevels     = 1;
    info.arrayLayers   = 1;
    info.samples       = VK_SAMPLE_COUNT_1_BIT;
    info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    info.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    err =
      vkCreateImage (g_Device, &info, g_Allocator, &g_FontImage);

    ImGui_ImplVulkan_VkResult (err);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements (g_Device, g_FontImage, &req);

    VkMemoryAllocateInfo alloc_info = { };

    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);

    err =
      vkAllocateMemory (g_Device, &alloc_info, g_Allocator, &g_FontMemory);

    ImGui_ImplVulkan_VkResult (err);

    err =
      vkBindImageMemory (g_Device, g_FontImage, g_FontMemory, 0);

    ImGui_ImplVulkan_VkResult (err);
  }

  // Create the Image View:
  {
    VkImageViewCreateInfo info = { };

    info.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                       = g_FontImage;
    info.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    info.format                      = VK_FORMAT_R8G8B8A8_UNORM;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;

    err =
      vkCreateImageView (g_Device, &info, g_Allocator, &g_FontView);

    ImGui_ImplVulkan_VkResult (err);
  }

  // Update the Descriptor Set:
  {
    VkDescriptorImageInfo desc_image [1] = { };

    desc_image [0].sampler     = g_FontSampler;
    desc_image [0].imageView   = g_FontView;
    desc_image [0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_desc [1] = { };

    write_desc [0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc [0].dstSet          = g_DescriptorSet;
    write_desc [0].descriptorCount = 1;
    write_desc [0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_desc [0].pImageInfo      = desc_image;

    vkUpdateDescriptorSets (g_Device, 1, write_desc, 0, NULL);
  }

  // Create the Upload Buffer:
  {
    VkBufferCreateInfo buffer_info = { };

    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = upload_size;
    buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    err =
      vkCreateBuffer (g_Device, &buffer_info, g_Allocator, &g_UploadBuffer);

    ImGui_ImplVulkan_VkResult (err);

    VkMemoryRequirements req;

    vkGetBufferMemoryRequirements (g_Device, g_UploadBuffer, &req);

    g_BufferMemoryAlignment = (g_BufferMemoryAlignment > req.alignment) ? g_BufferMemoryAlignment :
                                                                          req.alignment;

    VkMemoryAllocateInfo alloc_info = { };

    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVulkan_MemoryType (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

    err =
      vkAllocateMemory (g_Device, &alloc_info, g_Allocator, &g_UploadBufferMemory);

    ImGui_ImplVulkan_VkResult (err);

    err =
      vkBindBufferMemory (g_Device, g_UploadBuffer, g_UploadBufferMemory, 0);

    ImGui_ImplVulkan_VkResult (err);
  }

  // Upload to Buffer:
  {
    char* map = nullptr;

    err =
      vkMapMemory (g_Device, g_UploadBufferMemory, 0, upload_size, 0, (void**)(&map));

    ImGui_ImplVulkan_VkResult (err);

    memcpy (map, pixels, upload_size);

    VkMappedMemoryRange range [1] = { };

    range [0].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range [0].memory = g_UploadBufferMemory;
    range [0].size   = upload_size;

    err =
      vkFlushMappedMemoryRanges (g_Device, 1, range);

    ImGui_ImplVulkan_VkResult (err);

    vkUnmapMemory (g_Device, g_UploadBufferMemory);
  }
  // Copy to Image:
  {
    VkImageMemoryBarrier copy_barrier [1] = { };

    copy_barrier [0].sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copy_barrier [0].dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    copy_barrier [0].oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    copy_barrier [0].newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_barrier [0].srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier [0].dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier [0].image                       = g_FontImage;
    copy_barrier [0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_barrier [0].subresourceRange.levelCount = 1;
    copy_barrier [0].subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier (command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

    VkBufferImageCopy region = { };

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width           = width;
    region.imageExtent.height          = height;

    vkCmdCopyBufferToImage (command_buffer, g_UploadBuffer, g_FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier use_barrier [1] = { };

    use_barrier [0].sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier [0].srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier [0].dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
    use_barrier [0].oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier [0].newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier [0].srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    use_barrier [0].dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    use_barrier [0].image                       = g_FontImage;
    use_barrier [0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier [0].subresourceRange.levelCount = 1;
    use_barrier [0].subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier (command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
  }

  // Store our identifier
  io.Fonts->TexID =
    (void *)(intptr_t)g_FontImage;

  return true;
}

bool
ImGui_ImplVulkan_CreateDeviceObjects (void)
{
  VkResult       err;
  VkShaderModule vert_module;
  VkShaderModule frag_module;

  // Create The Shader Modules:
  {
    VkShaderModuleCreateInfo vert_info = { };

    vert_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_info.codeSize = sizeof(__glsl_shader_vert_spv);
    vert_info.pCode    = (uint32_t*)__glsl_shader_vert_spv;

    err =
      vkCreateShaderModule (g_Device, &vert_info, g_Allocator, &vert_module);

    ImGui_ImplVulkan_VkResult (err);

    VkShaderModuleCreateInfo frag_info = { };

    frag_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_info.codeSize = sizeof(__glsl_shader_frag_spv);
    frag_info.pCode    = (uint32_t*)__glsl_shader_frag_spv;

    err =
      vkCreateShaderModule (g_Device, &frag_info, g_Allocator, &frag_module);

    ImGui_ImplVulkan_VkResult (err);
  }

  if (! g_FontSampler)
  {
    VkSamplerCreateInfo info = { };

    info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter    = VK_FILTER_LINEAR;
    info.minFilter    = VK_FILTER_LINEAR;
    info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.minLod       = -1000;
    info.maxLod       =  1000;

    err =
      vkCreateSampler (g_Device, &info, g_Allocator, &g_FontSampler);

    ImGui_ImplVulkan_VkResult (err);
  }

  if (! g_DescriptorSetLayout)
  {
    VkSampler                    sampler [1] = { g_FontSampler };
    VkDescriptorSetLayoutBinding binding [1] = {               };

    binding [0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding [0].descriptorCount    = 1;
    binding [0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    binding [0].pImmutableSamplers = sampler;
    VkDescriptorSetLayoutCreateInfo info = {};

    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings    = binding;

    err =
      vkCreateDescriptorSetLayout (g_Device, &info, g_Allocator, &g_DescriptorSetLayout);

    ImGui_ImplVulkan_VkResult (err);
  }

  // Create Descriptor Set:
  {
    VkDescriptorSetAllocateInfo alloc_info = { };

    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = g_DescriptorPool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &g_DescriptorSetLayout;

    err =
      vkAllocateDescriptorSets (g_Device, &alloc_info, &g_DescriptorSet);

    ImGui_ImplVulkan_VkResult (err);
  }

  if (! g_PipelineLayout)
  {
    VkPushConstantRange push_constants [1] = { };

    push_constants [0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants [0].offset     = sizeof (float) * 0;
    push_constants [0].size       = sizeof (float) * 4;

    VkDescriptorSetLayout      set_layout [1] = { g_DescriptorSetLayout };
    VkPipelineLayoutCreateInfo layout_info    = {                       };

    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = 1;
    layout_info.pSetLayouts            = set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges     = push_constants;

    err =
      vkCreatePipelineLayout (g_Device, &layout_info, g_Allocator, &g_PipelineLayout);

    ImGui_ImplVulkan_VkResult (err);
  }

  VkPipelineShaderStageCreateInfo stage [2] = { };

  stage [0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage [0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
  stage [0].module = vert_module;
  stage [0].pName  = "main";
  stage [1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage [1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  stage [1].module = frag_module;
  stage [1].pName  = "main";

  VkVertexInputBindingDescription binding_desc [1] = { };

  binding_desc [0].stride    = sizeof(ImDrawVert);
  binding_desc [0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attribute_desc [3] = { };

  attribute_desc [0].location = 0;
  attribute_desc [0].binding  = binding_desc[0].binding;
  attribute_desc [0].format   = VK_FORMAT_R32G32_SFLOAT;
  attribute_desc [0].offset   = (size_t)(&((ImDrawVert*)0)->pos);
  attribute_desc [1].location = 1;
  attribute_desc [1].binding  = binding_desc[0].binding;
  attribute_desc [1].format   = VK_FORMAT_R32G32_SFLOAT;
  attribute_desc [1].offset   = (size_t)(&((ImDrawVert*)0)->uv);
  attribute_desc [2].location = 2;
  attribute_desc [2].binding  = binding_desc[0].binding;
  attribute_desc [2].format   = VK_FORMAT_R8G8B8A8_UNORM;
  attribute_desc [2].offset   = (size_t)(&((ImDrawVert*)0)->col);

  VkPipelineVertexInputStateCreateInfo vertex_info = { };

  vertex_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_info.vertexBindingDescriptionCount   = 1;
  vertex_info.pVertexBindingDescriptions      = binding_desc;
  vertex_info.vertexAttributeDescriptionCount = 3;
  vertex_info.pVertexAttributeDescriptions    = attribute_desc;

  VkPipelineInputAssemblyStateCreateInfo ia_info = { };

  ia_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo viewport_info = { };

  viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_info.viewportCount = 1;
  viewport_info.scissorCount  = 1;

  VkPipelineRasterizationStateCreateInfo raster_info = { };

  raster_info.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster_info.polygonMode = VK_POLYGON_MODE_FILL;
  raster_info.cullMode    = VK_CULL_MODE_NONE;
  raster_info.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  raster_info.lineWidth   = 1.0f;

  VkPipelineMultisampleStateCreateInfo ms_info = { };

  ms_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_attachment [1] = { };

  color_attachment [0].blendEnable         = VK_TRUE;
  color_attachment [0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_attachment [0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_attachment [0].colorBlendOp        = VK_BLEND_OP_ADD;
  color_attachment [0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_attachment [0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_attachment [0].alphaBlendOp        = VK_BLEND_OP_ADD;
  color_attachment [0].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineDepthStencilStateCreateInfo depth_info = { };

  depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  VkPipelineColorBlendStateCreateInfo blend_info = { };

  blend_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blend_info.attachmentCount = 1;
  blend_info.pAttachments    = color_attachment;

  const VkDynamicState dynamic_states [2] =
    { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

  VkPipelineDynamicStateCreateInfo dynamic_state = { };

  dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates    = dynamic_states;

  VkGraphicsPipelineCreateInfo info = { };

  info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info.flags               = g_PipelineCreateFlags;
  info.stageCount          = 2;
  info.pStages             = stage;
  info.pVertexInputState   = &vertex_info;
  info.pInputAssemblyState = &ia_info;
  info.pViewportState      = &viewport_info;
  info.pRasterizationState = &raster_info;
  info.pMultisampleState   = &ms_info;
  info.pDepthStencilState  = &depth_info;
  info.pColorBlendState    = &blend_info;
  info.pDynamicState       = &dynamic_state;
  info.layout              = g_PipelineLayout;
  info.renderPass          = g_RenderPass;

  err =
    vkCreateGraphicsPipelines (g_Device, g_PipelineCache, 1, &info, g_Allocator, &g_Pipeline);

  ImGui_ImplVulkan_VkResult (err);

  vkDestroyShaderModule (g_Device, vert_module, g_Allocator);
  vkDestroyShaderModule (g_Device, frag_module, g_Allocator);

  return true;
}

void
ImGui_ImplVulkan_InvalidateFontUploadObjects (void)
{
  if (g_UploadBuffer)
  {
    vkDestroyBuffer (g_Device, g_UploadBuffer, g_Allocator);
    g_UploadBuffer = VK_NULL_HANDLE;
  }

  if (g_UploadBufferMemory)
  {
    vkFreeMemory (g_Device, g_UploadBufferMemory, g_Allocator);
    g_UploadBufferMemory = VK_NULL_HANDLE;
  }
}

void
ImGui_ImplVulkan_InvalidateDeviceObjects (void)
{
  ImGui_ImplVulkan_InvalidateFontUploadObjects ();

  for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
  {
    if (g_VertexBuffer       [i])   { vkDestroyBuffer (g_Device, g_VertexBuffer       [i], g_Allocator); g_VertexBuffer       [i] = VK_NULL_HANDLE; }
    if (g_VertexBufferMemory [i])   { vkFreeMemory    (g_Device, g_VertexBufferMemory [i], g_Allocator); g_VertexBufferMemory [i] = VK_NULL_HANDLE; }
    if (g_IndexBuffer        [i])   { vkDestroyBuffer (g_Device, g_IndexBuffer        [i], g_Allocator); g_IndexBuffer        [i] = VK_NULL_HANDLE; }
    if (g_IndexBufferMemory  [i])   { vkFreeMemory    (g_Device, g_IndexBufferMemory  [i], g_Allocator); g_IndexBufferMemory  [i] = VK_NULL_HANDLE; }
  }

  if (g_FontView)             { vkDestroyImageView           (g_Device, g_FontView,            g_Allocator); g_FontView            = VK_NULL_HANDLE; }
  if (g_FontImage)            { vkDestroyImage               (g_Device, g_FontImage,           g_Allocator); g_FontImage           = VK_NULL_HANDLE; }
  if (g_FontMemory)           { vkFreeMemory                 (g_Device, g_FontMemory,          g_Allocator); g_FontMemory          = VK_NULL_HANDLE; }
  if (g_FontSampler)          { vkDestroySampler             (g_Device, g_FontSampler,         g_Allocator); g_FontSampler         = VK_NULL_HANDLE; }
  if (g_DescriptorSetLayout)  { vkDestroyDescriptorSetLayout (g_Device, g_DescriptorSetLayout, g_Allocator); g_DescriptorSetLayout = VK_NULL_HANDLE; }
  if (g_PipelineLayout)       { vkDestroyPipelineLayout      (g_Device, g_PipelineLayout,      g_Allocator); g_PipelineLayout      = VK_NULL_HANDLE; }
  if (g_Pipeline)             { vkDestroyPipeline            (g_Device, g_Pipeline,            g_Allocator); g_Pipeline            = VK_NULL_HANDLE; }

  extern void
  SK_ImGui_ResetExternal (void);
  SK_ImGui_ResetExternal ();
}

#define SK_IMPORT_VK_FUNC(name) {                                                      \
  name = (PFN_##name) SK_GetProcAddress (SK_GetModuleHandle (L"vulkan-1.dll"), #name); \
}

bool
ImGui_ImplVulkan_Init (ImGui_ImplVulkan_Init_Data* init_data)
{
  g_Allocator      = init_data->allocator;
  g_Gpu            = init_data->gpu;
  g_Device         = init_data->device;
  g_RenderPass     = init_data->render_pass;
  g_PipelineCache  = init_data->pipeline_cache;
  g_DescriptorPool = init_data->descriptor_pool;
  g_CheckVkResult  = init_data->check_vk_result;

  static bool first = true;

  if (first) {
    if (! QueryPerformanceFrequency ((LARGE_INTEGER *)&g_TicksPerSecond))
      return false;

    if (! SK_QueryPerformanceCounter ((LARGE_INTEGER *)&g_Time))
      return false;

    first = false;
  }

  SK_IMPORT_VK_FUNC (vkGetPhysicalDeviceMemoryProperties)
  SK_IMPORT_VK_FUNC (vkAllocateMemory)
  SK_IMPORT_VK_FUNC (vkFreeMemory)
  SK_IMPORT_VK_FUNC (vkMapMemory)
  SK_IMPORT_VK_FUNC (vkUnmapMemory)
  SK_IMPORT_VK_FUNC (vkFlushMappedMemoryRanges)
  SK_IMPORT_VK_FUNC (vkBindBufferMemory)
  SK_IMPORT_VK_FUNC (vkBindImageMemory)
  SK_IMPORT_VK_FUNC (vkGetBufferMemoryRequirements)
  SK_IMPORT_VK_FUNC (vkGetImageMemoryRequirements)
  SK_IMPORT_VK_FUNC (vkCreateBuffer)
  SK_IMPORT_VK_FUNC (vkDestroyBuffer)
  SK_IMPORT_VK_FUNC (vkCreateImage)
  SK_IMPORT_VK_FUNC (vkDestroyImage)
  SK_IMPORT_VK_FUNC (vkCreateImageView)
  SK_IMPORT_VK_FUNC (vkDestroyImageView)
  SK_IMPORT_VK_FUNC (vkCreateShaderModule)
  SK_IMPORT_VK_FUNC (vkDestroyShaderModule)
  SK_IMPORT_VK_FUNC (vkCreateGraphicsPipelines)
  SK_IMPORT_VK_FUNC (vkDestroyPipeline)
  SK_IMPORT_VK_FUNC (vkCreatePipelineLayout)
  SK_IMPORT_VK_FUNC (vkDestroyPipelineLayout)
  SK_IMPORT_VK_FUNC (vkCreateSampler)
  SK_IMPORT_VK_FUNC (vkDestroySampler)
  SK_IMPORT_VK_FUNC (vkCreateDescriptorSetLayout)
  SK_IMPORT_VK_FUNC (vkDestroyDescriptorSetLayout)
  SK_IMPORT_VK_FUNC (vkAllocateDescriptorSets)
  SK_IMPORT_VK_FUNC (vkUpdateDescriptorSets)
  SK_IMPORT_VK_FUNC (vkCmdBindPipeline)
  SK_IMPORT_VK_FUNC (vkCmdSetViewport)
  SK_IMPORT_VK_FUNC (vkCmdSetScissor)
  SK_IMPORT_VK_FUNC (vkCmdBindDescriptorSets)
  SK_IMPORT_VK_FUNC (vkCmdBindIndexBuffer)
  SK_IMPORT_VK_FUNC (vkCmdBindVertexBuffers)
  SK_IMPORT_VK_FUNC (vkCmdDrawIndexed)
  SK_IMPORT_VK_FUNC (vkCmdCopyBufferToImage)
  SK_IMPORT_VK_FUNC (vkCmdPipelineBarrier)
  SK_IMPORT_VK_FUNC (vkCmdPushConstants)

  ImGuiIO& io =
    ImGui::GetIO ();

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
  io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
  io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
  io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
  io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
  io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
  io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
  io.KeyMap [ImGuiKey_Home]       = VK_HOME;
  io.KeyMap [ImGuiKey_End]        = VK_END;
  io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
  io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
  io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
  io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
  io.KeyMap [ImGuiKey_A]          = 'A';
  io.KeyMap [ImGuiKey_C]          = 'C';
  io.KeyMap [ImGuiKey_V]          = 'V';
  io.KeyMap [ImGuiKey_X]          = 'X';
  io.KeyMap [ImGuiKey_Y]          = 'Y';
  io.KeyMap [ImGuiKey_Z]          = 'Z';

  io.ClipboardUserData = game_window.hWnd;
//io.ImeWindowHandle   = game_window.hWnd;

  ImGui_ImplVulkan_CreateDeviceObjects ();

  return true;
}


#include <SpecialK/render/backend.h>

void
ImGui_ImplVulkan_Shutdown (void)
{
  ImGui_ImplVulkan_InvalidateDeviceObjects ();
  ImGui::Shutdown ();
}

void
ImGui_ImplVulkan_NewFrame (void)
{
  ImGuiIO& io (ImGui::GetIO ());

  // Setup display size (every frame to accommodate for window resizing)
  int         w,         h;
  int display_w, display_h;

  RECT client;
  GetClientRect (SK_GetCurrentRenderBackend ().windows.device, &client);

  w = client.right  - client.left;
  h = client.bottom - client.top;

  display_w = w;
  display_h = h;

  io.DisplaySize             = ImVec2 ((float)w, (float)h);
  io.DisplayFramebufferScale = ImVec2 ((float)w, (float)h);//ImVec2 (w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

  // Setup time step
  INT64 current_time;

  SK_QueryPerformanceCounter ((LARGE_INTEGER *)&current_time);

  io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
  g_Time       =         current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;
}

void
ImGui_ImplVulkan_Render (VkCommandBuffer command_buffer)
{
  g_CommandBuffer = command_buffer;

  ImGui::Render ();

  g_CommandBuffer = VK_NULL_HANDLE;
  g_FrameIndex    = (g_FrameIndex + 1) % IMGUI_VK_QUEUED_FRAMES;
}
