////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_VKFACTORY_H__
#define __RENDER_VKFACTORY_H__


#include <NsCore/Noesis.h>
#include <NsCore/Delegate.h>
#include <NsRender/VKRenderDeviceApi.h>

#include "vulkan/vulkan_core.h"


namespace Noesis
{
class RenderDevice;
class Texture;
template<class T> class Ptr;
}

namespace NoesisApp
{

struct NS_RENDER_VKRENDERDEVICE_API VKFactory
{
    typedef Noesis::Delegate<void (VkCommandBuffer commandBuffer)> QueueSubmitFunc;

    struct InstanceInfo
    {
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0xFFFFFFFF;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = VK_NULL_HANDLE;

        // When enabled, pipelines supporting stereo rendering are compiled. This requires
        // GL_EXT_multiview (Android) or GL_ARB_shader_viewport_layer_array (Desktop)
        bool stereoSupport = false;

        // Used for command buffer submission to the the Vulkan graphics queue
        QueueSubmitFunc QueueSubmit;
    };

    static Noesis::Ptr<Noesis::RenderDevice> CreateDevice(bool sRGB, const InstanceInfo& info);

    struct RecordingInfo
    {
        // Vulkan command buffer that is currently recorded
        VkCommandBuffer commandBuffer;

        // Can be used to track lifetime of own resources
        uint64_t frameNumber;

        // All resources that were used in this frame (or before) are safe to be released
        uint64_t safeFrameNumber;
    };

    static void SetCommandBuffer(Noesis::RenderDevice* device, const RecordingInfo& info);
    static void SetRenderPass(Noesis::RenderDevice* device, VkRenderPass renderPass, uint32_t sampleCount);
    static void WarmUpRenderPass(Noesis::RenderDevice* device, VkRenderPass renderPass, uint32_t sampleCount);

    static Noesis::Ptr<Noesis::Texture> WrapTexture(VkImage image, uint32_t width, uint32_t height,
        uint32_t levels, VkFormat format, VkImageLayout layout, bool isInverted, bool hasAlpha);
    static void UpdateLayout(Noesis::Texture* texture, VkImageLayout layout);

    static void* CreatePixelShader(Noesis::RenderDevice* device, const char* label, uint8_t shader,
        const void* spirv, uint32_t size);
    static void ClearPixelShaders(Noesis::RenderDevice* device);
};

}

#endif
