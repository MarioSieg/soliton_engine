////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "VKRenderDevice.h"
#include "Shaders.h"

#include <NsCore/Log.h>
#include <NsCore/HighResTimer.h>
#include <NsCore/DynamicCast.h>
#include <NsCore/String.h>
#include <NsApp/FastLZ.h>
#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>

#include <stdio.h>
#include <inttypes.h>


using namespace Noesis;
using namespace NoesisApp;


// In Noesis, render targets are never fully used. They are always loaded with 'LOAD_OP_DONT_CARE'.
// When doing multisampling it is very important to only resolve affected regions. On tiled
// architectures, we can resolve the whole surface on writeback because only touched tiled will be
// really resolved. For desktop, we need to manually resolve each region with 'vkCmdResolveImage'.

#ifdef NS_PLATFORM_ANDROID
  #define RESOLVE_ON_WRITEBACK 1
#else
  #define RESOLVE_ON_WRITEBACK 0
#endif

#define DESCRIPTOR_POOL_MAX_SETS 128
#define PREALLOCATED_DYNAMIC_PAGES 2
#define CBUFFER_SIZE 16 * 1024

#define V(exp) \
    NS_MACRO_BEGIN \
        VkResult err_ = (exp); \
        NS_ASSERT(err_ == VK_SUCCESS); \
    NS_MACRO_END

#define LOAD_INSTANCE_FUNC(x) \
    NS_MACRO_BEGIN \
        NS_ASSERT(mInstance != nullptr); \
        x = (decltype(x))vkGetInstanceProcAddr(mInstance, #x); \
        NS_ASSERT(x != nullptr); \
    NS_MACRO_END

#define LOAD_INSTANCE_EXT_FUNC(x) \
    NS_MACRO_BEGIN \
        NS_ASSERT(mInstance != nullptr); \
        x = (decltype(x))vkGetInstanceProcAddr(mInstance, #x); \
    NS_MACRO_END

#define LOAD_DEVICE_FUNC(x) \
    NS_MACRO_BEGIN \
        NS_ASSERT(mDevice != nullptr); \
        x = (decltype(x))vkGetDeviceProcAddr(mDevice, #x); \
        NS_ASSERT(x != nullptr); \
    NS_MACRO_END

#define LOAD_DEVICE_EXT_FUNC(x) \
    NS_MACRO_BEGIN \
        NS_ASSERT(mDevice != nullptr); \
        x = (decltype(x))vkGetDeviceProcAddr(mDevice, #x); \
    NS_MACRO_END

#ifdef NS_PROFILE
    #define VK_BEGIN_EVENT(...) \
        NS_MACRO_BEGIN \
            if (vkCmdDebugMarkerBeginEXT != nullptr) \
            { \
                char name[128]; \
                snprintf(name, sizeof(name), __VA_ARGS__); \
                VkDebugMarkerMarkerInfoEXT info{}; \
                info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT; \
                info.pMarkerName = name; \
                vkCmdDebugMarkerBeginEXT(mCommandBuffer, &info); \
            } \
            else if (vkCmdBeginDebugUtilsLabelEXT != nullptr) \
            { \
                char name[128]; \
                snprintf(name, sizeof(name), __VA_ARGS__); \
                VkDebugUtilsLabelEXT info{}; \
                info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT; \
                info.pLabelName = name; \
                vkCmdBeginDebugUtilsLabelEXT(mCommandBuffer, &info); \
            } \
        NS_MACRO_END
    #define VK_END_EVENT() \
        NS_MACRO_BEGIN \
            if (vkCmdDebugMarkerEndEXT != nullptr) \
            { \
                vkCmdDebugMarkerEndEXT(mCommandBuffer); \
            } \
            else if (vkCmdEndDebugUtilsLabelEXT != nullptr) \
            { \
                vkCmdEndDebugUtilsLabelEXT(mCommandBuffer); \
            } \
        NS_MACRO_END
    #define VK_NAME(obj, type, ...) \
        NS_MACRO_BEGIN \
            NS_ASSERT(mDevice != nullptr); \
            if (vkDebugMarkerSetObjectNameEXT != nullptr) \
            { \
                char name[128]; \
                snprintf(name, sizeof(name), __VA_ARGS__); \
                VkDebugMarkerObjectNameInfoEXT info{}; \
                info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT; \
                info.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_##type##_EXT; \
                info.object = (uint64_t)obj; \
                info.pObjectName = name; \
                V(vkDebugMarkerSetObjectNameEXT(mDevice, &info)); \
            } \
            else if (vkSetDebugUtilsObjectNameEXT != nullptr) \
            { \
                char name[128]; \
                snprintf(name, sizeof(name), __VA_ARGS__); \
                VkDebugUtilsObjectNameInfoEXT info{}; \
                info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT; \
                info.objectType = VK_OBJECT_TYPE_##type; \
                info.objectHandle = (uint64_t)obj; \
                info.pObjectName = name; \
                V(vkSetDebugUtilsObjectNameEXT(mDevice, &info)); \
            } \
        NS_MACRO_END
#else
    #define VK_BEGIN_EVENT(...) NS_NOOP
    #define VK_END_EVENT() NS_NOOP
    #define VK_NAME(obj, type, ...) NS_UNUSED(__VA_ARGS__)
#endif

namespace
{

// Root signature flags
static const uint32_t VS_CB0 = 1;
static const uint32_t VS_CB1 = 2;
static const uint32_t PS_CB0 = 4;
static const uint32_t PS_CB1 = 8;
static const uint32_t PS_T0 = 16;
static const uint32_t PS_T1 = 32;
static const uint32_t PS_T2 = 64;
static const uint32_t PS_T3 = 128;
static const uint32_t PS_T4 = 256;

struct ShaderVS
{
    const char* label;
    uint32_t start;
    uint32_t size;
};

#ifdef NS_PLATFORM_ANDROID
    #define VSHADER(n) case Shader::Vertex::n: \
        return stereo ? \
            ShaderVS { #n, n##_Multiview_VS_Start, n##_Multiview_VS_Size } : \
            ShaderVS { #n, n##_VS_Start, n##_VS_Size };

    #define VSHADER_SRGB(n) case Shader::Vertex::n: \
        return stereo ? \
            (sRGB ? \
                ShaderVS{ #n"_sRGB", n##_SRGB_Multiview_VS_Start, n##_SRGB_Multiview_VS_Size } : \
                ShaderVS{ #n, n##_Multiview_VS_Start, n##_Multiview_VS_Size }) : \
            (sRGB ? \
                ShaderVS{ #n"_sRGB", n##_SRGB_VS_Start, n##_SRGB_VS_Size } : \
                ShaderVS{ #n, n##_VS_Start, n##_VS_Size });
#else
    #define VSHADER(n) case Shader::Vertex::n: \
        return stereo ? \
            ShaderVS { #n, n##_Layer_VS_Start, n##_Layer_VS_Size } : \
            ShaderVS { #n, n##_VS_Start, n##_VS_Size };

    #define VSHADER_SRGB(n) case Shader::Vertex::n: \
        return stereo ? \
            (sRGB ? \
                ShaderVS{ #n"_sRGB", n##_SRGB_Layer_VS_Start, n##_SRGB_Layer_VS_Size } : \
                ShaderVS{ #n, n##_Layer_VS_Start, n##_Layer_VS_Size }) : \
            (sRGB ? \
                ShaderVS{ #n"_sRGB", n##_SRGB_VS_Start, n##_SRGB_VS_Size } : \
                ShaderVS{ #n, n##_VS_Start, n##_VS_Size });
#endif

static auto ShadersVS = [](uint32_t shader, bool sRGB, bool stereo)
{
    switch (shader)
    {
        VSHADER(Pos)
        VSHADER_SRGB(PosColor)
        VSHADER(PosTex0)
        VSHADER(PosTex0Rect)
        VSHADER(PosTex0RectTile)
        VSHADER_SRGB(PosColorCoverage)
        VSHADER(PosTex0Coverage)
        VSHADER(PosTex0CoverageRect)
        VSHADER(PosTex0CoverageRectTile)
        VSHADER_SRGB(PosColorTex1_SDF)
        VSHADER(PosTex0Tex1_SDF)
        VSHADER(PosTex0Tex1Rect_SDF)
        VSHADER(PosTex0Tex1RectTile_SDF)
        VSHADER_SRGB(PosColorTex1)
        VSHADER(PosTex0Tex1)
        VSHADER(PosTex0Tex1Rect)
        VSHADER(PosTex0Tex1RectTile)
        VSHADER_SRGB(PosColorTex0Tex1)
        VSHADER(PosTex0Tex1_Downsample)
        VSHADER_SRGB(PosColorTex1Rect)
        VSHADER_SRGB(PosColorTex0RectImagePos)

        default: NS_ASSERT_UNREACHABLE;
    }
};

struct ShaderPS
{
    const char* label;
    uint32_t start;
    uint32_t size;
    uint32_t signature;
};

#define PSHADER(n, s) case Shader::n: return ShaderPS { #n, n##_PS_Start, n##_PS_Size, s };

static auto ShadersPS = [](uint32_t shader)
{
    switch (shader)
    {
        PSHADER(RGBA, VS_CB0 | PS_CB0)
        PSHADER(Mask, VS_CB0)
        PSHADER(Clear, VS_CB0)

        PSHADER(Path_Solid, VS_CB0)
        PSHADER(Path_Linear, VS_CB0 | PS_CB0 | PS_T1)
        PSHADER(Path_Radial, VS_CB0 | PS_CB0 | PS_T1)
        PSHADER(Path_Pattern, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0)

        PSHADER(Path_AA_Solid, VS_CB0)
        PSHADER(Path_AA_Linear, VS_CB0 | PS_CB0 | PS_T1)
        PSHADER(Path_AA_Radial, VS_CB0 | PS_CB0 | PS_T1)
        PSHADER(Path_AA_Pattern, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_AA_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_AA_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_AA_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_AA_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0)
        PSHADER(Path_AA_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0)

        PSHADER(SDF_Solid, VS_CB0 | VS_CB1 | PS_T3)
        PSHADER(SDF_Linear, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
        PSHADER(SDF_Radial, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
        PSHADER(SDF_Pattern, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
        PSHADER(SDF_Pattern_Clamp, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
        PSHADER(SDF_Pattern_Repeat, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
        PSHADER(SDF_Pattern_MirrorU, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
        PSHADER(SDF_Pattern_MirrorV, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
        PSHADER(SDF_Pattern_Mirror, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)

        PSHADER(Opacity_Solid, VS_CB0 | PS_T2)
        PSHADER(Opacity_Linear, VS_CB0 | PS_CB0 | PS_T1 | PS_T2)
        PSHADER(Opacity_Radial, VS_CB0 | PS_CB0 | PS_T1 | PS_T2)
        PSHADER(Opacity_Pattern, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
        PSHADER(Opacity_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
        PSHADER(Opacity_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
        PSHADER(Opacity_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
        PSHADER(Opacity_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
        PSHADER(Opacity_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)

        PSHADER(Upsample, VS_CB0 | PS_T0 | PS_T2)
        PSHADER(Downsample, VS_CB0 | PS_T0)

        PSHADER(Shadow, VS_CB0 | PS_CB1 | PS_T2 | PS_T4)
        PSHADER(Blur, VS_CB0 | PS_CB1 | PS_T2 | PS_T4)

        default: return ShaderPS{};
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class VKTexture final: public Texture
{
public:
    VKTexture(uint32_t hash_): hash(hash_) {}

    ~VKTexture()
    {
        if (device)
        {
            device->SafeReleaseTexture(this);
        }
    }

    uint32_t GetWidth() const override { return width; }
    uint32_t GetHeight() const override { return height; }
    bool HasMipMaps() const override { return levels > 1; }
    bool IsInverted() const override { return isInverted; }
    bool HasAlpha() const override { return hasAlpha; }

    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t levels = 0;

    VKRenderDevice* device = VK_NULL_HANDLE;

    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    bool hasAlpha = true;
    bool isInverted = false;

    uint32_t hash = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class VKRenderTarget final: public RenderTarget
{
public:
    ~VKRenderTarget()
    {
        color->device->SafeReleaseRenderTarget(this);
    }

    Texture* GetTexture() override { return color; }

    Ptr<VKTexture> color;
    Ptr<VKTexture> colorAA;
    Ptr<VKTexture> stencil;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////
VKRenderDevice::VKRenderDevice(bool sRGB, const VKFactory::InstanceInfo& info)
{
    NS_ASSERT(info.instance);
    NS_ASSERT(info.physicalDevice);
    NS_ASSERT(info.device);
    NS_ASSERT(info.vkGetInstanceProcAddr);
    NS_ASSERT(info.QueueSubmit);

    mInstance = info.instance;
    mPhysicalDevice = info.physicalDevice;
    mDevice = info.device;
    mPipelineCache = info.pipelineCache;
    mQueueFamilyIndex = info.queueFamilyIndex;
    vkGetInstanceProcAddr = info.vkGetInstanceProcAddr;
    mStereoSupport = info.stereoSupport;
    mQueueSubmit = info.QueueSubmit;

    LOAD_INSTANCE_FUNC(vkGetDeviceProcAddr);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties);

    LOAD_INSTANCE_EXT_FUNC(vkCmdBeginDebugUtilsLabelEXT);
    LOAD_INSTANCE_EXT_FUNC(vkCmdEndDebugUtilsLabelEXT);
    LOAD_INSTANCE_EXT_FUNC(vkSetDebugUtilsObjectNameEXT);

    LOAD_DEVICE_FUNC(vkCreateBuffer);
    LOAD_DEVICE_FUNC(vkDestroyBuffer);
    LOAD_DEVICE_FUNC(vkCreateFramebuffer);
    LOAD_DEVICE_FUNC(vkDestroyFramebuffer);
    LOAD_DEVICE_FUNC(vkCreateRenderPass);
    LOAD_DEVICE_FUNC(vkDestroyRenderPass);
    LOAD_DEVICE_FUNC(vkGetBufferMemoryRequirements);
    LOAD_DEVICE_FUNC(vkGetImageMemoryRequirements);
    LOAD_DEVICE_FUNC(vkAllocateMemory);
    LOAD_DEVICE_FUNC(vkFreeMemory);
    LOAD_DEVICE_FUNC(vkBindBufferMemory);
    LOAD_DEVICE_FUNC(vkBindImageMemory);
    LOAD_DEVICE_FUNC(vkMapMemory);
    LOAD_DEVICE_FUNC(vkCmdBindVertexBuffers);
    LOAD_DEVICE_FUNC(vkCmdBindIndexBuffer);
    LOAD_DEVICE_FUNC(vkCreateDescriptorSetLayout);
    LOAD_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
    LOAD_DEVICE_FUNC(vkCreatePipelineLayout);
    LOAD_DEVICE_FUNC(vkDestroyPipelineLayout);
    LOAD_DEVICE_FUNC(vkCreateShaderModule);
    LOAD_DEVICE_FUNC(vkDestroyShaderModule);
    LOAD_DEVICE_FUNC(vkCreateGraphicsPipelines);
    LOAD_DEVICE_FUNC(vkCmdBindPipeline);
    LOAD_DEVICE_FUNC(vkDestroyPipeline);
    LOAD_DEVICE_FUNC(vkCreateDescriptorPool);
    LOAD_DEVICE_FUNC(vkResetDescriptorPool);
    LOAD_DEVICE_FUNC(vkDestroyDescriptorPool);
    LOAD_DEVICE_FUNC(vkAllocateDescriptorSets);
    LOAD_DEVICE_FUNC(vkUpdateDescriptorSets);
    LOAD_DEVICE_FUNC(vkCmdBindDescriptorSets);
    LOAD_DEVICE_FUNC(vkCmdDrawIndexed);
    LOAD_DEVICE_FUNC(vkCmdSetStencilReference);
    LOAD_DEVICE_FUNC(vkCreateImage);
    LOAD_DEVICE_FUNC(vkDestroyImage);
    LOAD_DEVICE_FUNC(vkCreateImageView);
    LOAD_DEVICE_FUNC(vkDestroyImageView);
    LOAD_DEVICE_FUNC(vkCreateSampler);
    LOAD_DEVICE_FUNC(vkDestroySampler);
    LOAD_DEVICE_FUNC(vkCmdPipelineBarrier);
    LOAD_DEVICE_FUNC(vkCmdCopyBufferToImage);
    LOAD_DEVICE_FUNC(vkCreateCommandPool);
    LOAD_DEVICE_FUNC(vkDestroyCommandPool);
    LOAD_DEVICE_FUNC(vkResetCommandPool);
    LOAD_DEVICE_FUNC(vkAllocateCommandBuffers);
    LOAD_DEVICE_FUNC(vkBeginCommandBuffer);
    LOAD_DEVICE_FUNC(vkEndCommandBuffer);
    LOAD_DEVICE_FUNC(vkCmdBeginRenderPass);
    LOAD_DEVICE_FUNC(vkCmdEndRenderPass);
    LOAD_DEVICE_FUNC(vkCmdSetViewport);
    LOAD_DEVICE_FUNC(vkCmdSetScissor);
    LOAD_DEVICE_FUNC(vkCmdResolveImage);
    LOAD_DEVICE_FUNC(vkQueueSubmit);

    LOAD_DEVICE_EXT_FUNC(vkCmdDebugMarkerBeginEXT);
    LOAD_DEVICE_EXT_FUNC(vkCmdDebugMarkerEndEXT);
    LOAD_DEVICE_EXT_FUNC(vkDebugMarkerSetObjectNameEXT);

    FillCaps(sRGB);

    CreateBuffers();
    CreateLayouts();
    CreateShaders();
    CreateSamplers();
    CreateTransferCommandPool();
    CreateDescriptorPool();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VKRenderDevice::~VKRenderDevice()
{
    ProcessPendingDestroys(true);
    NS_ASSERT(mPendingDestroys.Empty());

    DestroyRenderPasses();
    DestroyDescriptorPool();
    DestroyTransferCommandPool();
    DestroySamplers();
    DestroyPipelines();
    DestroyShaders();
    DestroyLayouts();
    DestroyBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SetCommandBuffer(const VKFactory::RecordingInfo& info)
{
    mCommandBuffer = info.commandBuffer;
    mFrameNumber = info.frameNumber;
    mSafeFrameNumber = info.safeFrameNumber;

    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SetRenderPass(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount)
{
    mActiveRenderPass = renderPass;
    CreatePipelines(renderPass, sampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::WarmUpRenderPass(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount)
{
    CreatePipelines(renderPass, sampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> VKRenderDevice::WrapTexture(VkImage image, uint32_t width, uint32_t height,
    uint32_t levels, VkFormat format, VkImageLayout layout, bool isInverted, bool hasAlpha)
{
    Ptr<VKTexture> texture = MakePtr<VKTexture>(0);

    texture->image = image;
    texture->format = format;
    texture->layout = layout;
    texture->width = width;
    texture->height = height;
    texture->levels = levels;
    texture->isInverted = isInverted;
    texture->hasAlpha = hasAlpha;

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::UpdateLayout(Texture* texture_, VkImageLayout layout)
{
    VKTexture* texture = (VKTexture*)texture_;
    texture->layout = layout;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKRenderDevice::CreatePixelShader(const char* label, uint8_t shader, const void* spirv,
    uint32_t size)
{
  #ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
  #endif

    uint32_t signature;
    memcpy(&signature, spirv, sizeof(signature));
    spirv = (uint8_t*)spirv + sizeof(signature);
    size -= sizeof(signature);

    VkShaderModule module;
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = (const uint32_t*)spirv;
    V(vkCreateShaderModule(mDevice, &createInfo, nullptr, &module));
    VK_NAME(module, SHADER_MODULE, "Noesis_%s", label);

    Layout layout;
    CreateLayout(signature, layout);

    mCustomShaders.PushBack({ label, shader, layout, module });

    for (const auto& renderPass : mCachedPipelineRenderPasses)
    {
        CreatePipelines(label, shader, renderPass.key, module, layout.pipelineLayout,
            renderPass.value, mCustomShaders.Size());
    }

  #ifdef NS_PROFILE
    uint64_t t1 = HighResTimer::Ticks();
    NS_LOG_TRACE("'%s' shader compiled in %.0f ms", label, 1000.0 * HighResTimer::Seconds(t1 - t0));
  #endif

    return (void*)(uintptr_t)mCustomShaders.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::ClearPixelShaders()
{
    for (CustomShader& shader : mCustomShaders)
    {
        vkDestroyShaderModule(mDevice, shader.module, nullptr);
    }

    mCustomShaders.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SafeReleaseTexture(Texture* texture_)
{
    VKTexture* texture = (VKTexture*)texture_;

    PendingDestroy p{};
    p.frame = mFrameNumber;
    p.memory = texture->memory;
    p.image = texture->image;
    p.view = texture->view;

    mPendingDestroys.PushBack(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SafeReleaseRenderTarget(RenderTarget* surface_)
{
    VKRenderTarget* surface = (VKRenderTarget*)surface_;

    PendingDestroy p{};
    p.frame = mFrameNumber;
    p.framebuffer = surface->framebuffer;

    mPendingDestroys.PushBack(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SafeReleaseBuffer(VkBuffer buffer, VkDeviceMemory memory)
{
    PendingDestroy p{};
    p.frame = mFrameNumber;
    p.buffer = buffer;
    p.memory = memory;

    mPendingDestroys.PushBack(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DeviceCaps& VKRenderDevice::GetCaps() const
{
    return mCaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static VkSampleCountFlagBits GetSampleCount(uint32_t samples, const VkPhysicalDeviceLimits& limits)
{
    auto sampleCounts = limits.framebufferColorSampleCounts & limits.framebufferStencilSampleCounts;

    for (uint32_t bits = VK_SAMPLE_COUNT_64_BIT; bits > VK_SAMPLE_COUNT_1_BIT; bits >>= 1)
    {
        if (samples >= bits && (sampleCounts & bits) > 0)
        {
            return (VkSampleCountFlagBits)bits;
        }
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t Index(VkSampleCountFlagBits samples)
{
    switch (samples)
    {
        case VK_SAMPLE_COUNT_1_BIT:  return 0;
        case VK_SAMPLE_COUNT_2_BIT:  return 1;
        case VK_SAMPLE_COUNT_4_BIT:  return 2;
        case VK_SAMPLE_COUNT_8_BIT:  return 3;
        case VK_SAMPLE_COUNT_16_BIT: return 4;
        case VK_SAMPLE_COUNT_32_BIT: return 5;
        case VK_SAMPLE_COUNT_64_BIT: return 6;
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> VKRenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, uint32_t samples_, bool needsStencil)
{
    Ptr<VKRenderTarget> surface = MakePtr<VKRenderTarget>();
    surface->samples = GetSampleCount(samples_, mDeviceProperties.limits);

    EnsureTransferCommands();

    Vector<VkImageView, 3> attachments;

    if (needsStencil)
    {
        surface->stencil = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mStencilFormat, surface->samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, VK_IMAGE_ASPECT_STENCIL_BIT));
        attachments.PushBack(surface->stencil->view);
        ChangeLayout(mTransferCommands, surface->stencil, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    if (surface->samples > 1)
    {
      #if RESOLVE_ON_WRITEBACK
        // Multisample surface
        surface->colorAA = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mBackBufferFormat, surface->samples, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->colorAA, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        attachments.PushBack(surface->colorAA->view);

        // Resolve surface
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mBackBufferFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.PushBack(surface->color->view);
      #else
        // Multisample surface
        surface->colorAA = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mBackBufferFormat, surface->samples, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->colorAA, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // Resolve surface
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mBackBufferFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT));
        attachments.PushBack(surface->colorAA->view);
      #endif
    }
    else
    {
        // XamlTester needs VK_IMAGE_USAGE_TRANSFER_SRC_BIT for grabbing screenshots
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            mBackBufferFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT));
        attachments.PushBack(surface->color->view);
        ChangeLayout(mTransferCommands, surface->color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    uint32_t index = Index(surface->samples);
    uint32_t stencilCount = needsStencil ? 1 : 0;
    NS_ASSERT(index < NS_COUNTOF(mRenderPasses));

    if (NS_UNLIKELY(mRenderPasses[index][stencilCount] == VK_NULL_HANDLE))
    {
        VkRenderPass renderPass = CreateRenderPass(surface->samples, needsStencil);
        CreatePipelines(renderPass, surface->samples);
        mRenderPasses[index][stencilCount] = renderPass;
    }

    surface->renderPass = mRenderPasses[index][stencilCount];

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = surface->renderPass;
    framebufferInfo.attachmentCount = attachments.Size();
    framebufferInfo.pAttachments = attachments.Data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    V(vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &surface->framebuffer));
    VK_NAME(surface->framebuffer, FRAMEBUFFER, "Noesis_%s_FrameBuffer", label);
    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> VKRenderDevice::CloneRenderTarget(const char* label, RenderTarget* surface_)
{
    VKRenderTarget* src = (VKRenderTarget*)surface_;

    uint32_t width = src->color->width;
    uint32_t height = src->color->height;

    Ptr<VKRenderTarget> surface = MakePtr<VKRenderTarget>();
    surface->renderPass = src->renderPass;
    surface->samples = src->samples;
    surface->colorAA = src->colorAA;
    surface->stencil = src->stencil;

    Vector<VkImageView, 3> attachments;
    if (surface->stencil) { attachments.PushBack(surface->stencil->view); }

    EnsureTransferCommands();

    if (src->samples > 1)
    {
      #if RESOLVE_ON_WRITEBACK
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            src->color->format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.PushBack(surface->colorAA->view);
        attachments.PushBack(surface->color->view);

      #else
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            src->color->format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.PushBack(surface->colorAA->view);
      #endif
    }
    else
    {
        surface->color = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, 1,
            src->color->format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT));
        ChangeLayout(mTransferCommands, surface->color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        attachments.PushBack(surface->color->view);
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = surface->renderPass;
    framebufferInfo.attachmentCount = attachments.Size();
    framebufferInfo.pAttachments = attachments.Data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    V(vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &surface->framebuffer));
    VK_NAME(surface->framebuffer, FRAMEBUFFER, "Noesis_%s_FrameBuffer", label);
    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static VkFormat VKFormat(TextureFormat::Enum format, bool sRGB)
{
    switch (format)
    {
        case TextureFormat::RGBA8: return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBX8: return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t Align(uint32_t n, uint32_t alignment)
{
    NS_ASSERT(IsPow2(alignment));
    return (n + (alignment - 1)) & ~(alignment - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> VKRenderDevice::CreateTexture(const char* label, uint32_t width, uint32_t height,
    uint32_t numLevels, TextureFormat::Enum format_, const void** data)
{
    VkFormat format = VKFormat(format_, mCaps.linearRendering);
    Ptr<VKTexture> texture = StaticPtrCast<VKTexture>(CreateTexture(label, width, height, numLevels,
        format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT));
    texture->hasAlpha = format_ == TextureFormat::RGBA8;

    if (data != nullptr)
    {
        uint32_t texelBytes = texture->format == VK_FORMAT_R8_UNORM ? 1 : 4;
        uint32_t bufferSize = 0;

        for (uint32_t i = 0; i < numLevels; i++)
        {
            uint32_t w = Max(width >> i, 1U);
            uint32_t h = Max(height >> i, 1U);

            bufferSize += Align(w * h * texelBytes, 4);
        }

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        V(vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkDeviceMemory memory;
        V(vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory));
        V(vkBindBufferMemory(mDevice, buffer, memory, 0));

        void* ptr;
        V(vkMapMemory(mDevice, memory, 0, VK_WHOLE_SIZE, 0, &ptr));

        uint32_t offset = 0;
        Vector<VkBufferImageCopy, 32> regions;

        for (uint32_t i = 0; i < numLevels; i++)
        {
            uint32_t w = Max(width >> i, 1U);
            uint32_t h = Max(height >> i, 1U);
            uint32_t size = w * h * texelBytes;

            VkBufferImageCopy region{};
            region.imageExtent.width = w;
            region.imageExtent.height = h;
            region.imageExtent.depth = 1;
            region.imageSubresource.mipLevel = i;
            region.bufferOffset = offset;
            region.imageSubresource.layerCount = 1;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            regions.PushBack(region);

            memcpy((uint8_t*)ptr + offset, data[i], size);
            offset += Align(size, 4);
        }

        EnsureTransferCommands();
        ChangeLayout(mTransferCommands, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyBufferToImage(mTransferCommands, buffer, texture->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.Size(), regions.Data());

        SafeReleaseBuffer(buffer, memory);
        mUpdatedTextures.PushBack(texture);
    }

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::UpdateTexture(Texture* texture_, uint32_t level, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, const void* data)
{
    VKTexture* texture = (VKTexture*)texture_;

    uint32_t texelBytes = texture->format == VK_FORMAT_R8_UNORM ? 1 : 4;
    uint32_t size = width * height * texelBytes;
    void* memory = MapBuffer(mTexUpload, Align(size, 4));
    memcpy(memory, data, size);

    EnsureTransferCommands();
    ChangeLayout(mTransferCommands, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageOffset.x = x;
    region.imageOffset.y = y;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    region.imageSubresource.mipLevel = level;
    region.bufferOffset = mTexUpload.drawPos;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCmdCopyBufferToImage(mTransferCommands, mTexUpload.currentPage->buffer, texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (mUpdatedTextures.Find(texture) == mUpdatedTextures.End())
    {
        mUpdatedTextures.PushBack(texture);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::BeginOffscreenRender()
{
    VK_BEGIN_EVENT("Noesis.Offscreen");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::EndOffscreenRender()
{
    FlushTransferCommands();
    VK_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::BeginOnscreenRender()
{
    ProcessPendingDestroys(false);
    VK_BEGIN_EVENT("Noesis");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::EndOnscreenRender()
{
    FlushTransferCommands();
    VK_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SetRenderTarget(RenderTarget* surface_)
{
    VKRenderTarget* surface = (VKRenderTarget*)surface_;
    VK_BEGIN_EVENT("SetRenderTarget");

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)surface->color->width;
    viewport.height = (float)surface->color->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::BeginTile(RenderTarget* surface_, const Tile& tile)
{
    VKRenderTarget* surface = (VKRenderTarget*)surface_;

    VkRect2D renderArea;
    renderArea.offset.x = tile.x;
    renderArea.offset.y = surface->color->height - (tile.y + tile.height);
    renderArea.extent.width = tile.width;
    renderArea.extent.height = tile.height;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = surface->renderPass;
    renderPassInfo.framebuffer = surface->framebuffer;
    renderPassInfo.renderArea = renderArea;

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetScissor(mCommandBuffer, 0, 1, &renderArea);

    mActiveRenderPass = surface->renderPass;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::EndTile(RenderTarget*)
{
    mActiveRenderPass = VK_NULL_HANDLE;
    vkCmdEndRenderPass(mCommandBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::ResolveRenderTarget(RenderTarget* surface_, const Tile* tiles, uint32_t numTiles)
{
    VK_END_EVENT();

  #if !RESOLVE_ON_WRITEBACK
    VKRenderTarget* surface = (VKRenderTarget*)surface_;

    if (surface->samples > 1)
    {
        Vector<VkImageResolve, 32> regions;

        VKTexture* src = surface->colorAA;
        VKTexture* dst = surface->color;

        for (uint32_t i = 0; i < numTiles; i++)
        {
            VkImageResolve& region = regions.EmplaceBack();

            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = 1;

            region.srcOffset.x = tiles[i].x;
            region.srcOffset.y = dst->height - tiles[i].y - tiles[i].height;
            region.srcOffset.z = 0;;

            region.dstSubresource = region.srcSubresource;
            region.dstOffset = region.srcOffset;

            region.extent.width = tiles[i].width;
            region.extent.height = tiles[i].height;
            region.extent.depth = 1;
        }

        ChangeLayout(mCommandBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        vkCmdResolveImage(mCommandBuffer, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.Size(), regions.Data());
        ChangeLayout(mCommandBuffer, dst,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  #else
    NS_UNUSED(surface_, tiles, numTiles);
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKRenderDevice::MapVertices(uint32_t bytes)
{
    return MapBuffer(mVertices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::UnmapVertices()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKRenderDevice::MapIndices(uint32_t bytes)
{
    return MapBuffer(mIndices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::UnmapIndices()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t GetSignature(const Batch& batch)
{
    uint32_t signature = 0;

    if (batch.pattern) signature |= PS_T0;
    if (batch.ramps) signature |= PS_T1;
    if (batch.image) signature |= PS_T2;
    if (batch.glyphs) signature |= PS_T3;
    if (batch.shadow) signature |= PS_T4;
    if (batch.vertexUniforms[0].values) signature |= VS_CB0;
    if (batch.vertexUniforms[1].values) signature |= VS_CB1;
    if (batch.pixelUniforms[0].values) signature |= PS_CB0;
    if (batch.pixelUniforms[1].values) signature |= PS_CB1;

    return signature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DrawBatch(const Batch& batch)
{
    NS_ASSERT(!batch.singlePassStereo || mStereoSupport);

    Layout layout;

    if (batch.pixelShader == 0)
    {
        NS_ASSERT(batch.shader.v < NS_COUNTOF(mLayouts));
        layout = mLayouts[batch.shader.v];
    }
    else
    {
        NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShaders.Size());
        layout = mCustomShaders[(int)(uintptr_t)batch.pixelShader - 1].layout;
    }

    // Skip draw if shader requires something not available in the batch
    if (NS_UNLIKELY((layout.signature & GetSignature(batch)) != layout.signature)) return;

    SetBuffers(batch);
    BindDescriptors(batch, layout);
    BindPipeline(batch);
    SetStencilRef(batch.stencilRef);

    uint32_t firstIndex = batch.startIndex + mIndices.drawPos / 2;

    if (batch.singlePassStereo)
    {
      #ifdef NS_PLATFORM_ANDROID
        // GL_EXT_multiview
        vkCmdDrawIndexed(mCommandBuffer, batch.numIndices, 1, firstIndex, 0, 0);
      #else
        // GL_ARB_shader_viewport_layer_array
        vkCmdDrawIndexed(mCommandBuffer, batch.numIndices, 2, firstIndex, 0, 0);
      #endif
    }
    else
    {
        vkCmdDrawIndexed(mCommandBuffer, batch.numIndices, 1, firstIndex, 0, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::FillCaps(bool sRGB)
{
    mCaps.centerPixelOffset = 0.0f;
    mCaps.linearRendering = sRGB;
    mCaps.clipSpaceYInverted = true;

    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);
    vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);

    mStencilFormat = FindStencilFormat();
    mBackBufferFormat = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::InvalidateStateCache()
{
    mCachedPipeline = VK_NULL_HANDLE;
    mCachedIndexBuffer = VK_NULL_HANDLE;
    mCachedStencilRef = 0xFFFFFFFF;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t VKRenderDevice::FindMemoryType(uint32_t typeBits, uint32_t required) const
{
    for (uint32_t i = 0; i < mMemoryProperties.memoryTypeCount; i++)
    {
        VkMemoryPropertyFlags flags = mMemoryProperties.memoryTypes[i].propertyFlags;
        if ((typeBits & (1 << i)) && ((flags & required) == required))
        {
            return i;
        }
    }

    return UINT32_MAX;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t VKRenderDevice::FindMemoryType(uint32_t typeBits, uint32_t optimal, uint32_t required) const
{
    uint32_t r = FindMemoryType(typeBits, optimal);

    if (r == UINT32_MAX)
    {
        r = FindMemoryType(typeBits, required);
    }

    NS_ASSERT(r != UINT32_MAX);
    return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VkFormat VKRenderDevice::FindStencilFormat() const
{
    NS_ASSERT(mPhysicalDevice != nullptr);

    VkFormatProperties props;

    // https://vulkan.gpuinfo.org/listoptimaltilingformats.php
    // ----------------------------------------------------
    // Format                       Windows  Linux  Android
    // ----------------------------------------------------
    // VK_FORMAT_S8_UINT                64%    85%     67%
    // VK_FORMAT_D32_SFLOAT_S8_UINT    100%    97%     46%
    // VK_FORMAT_D24_UNORM_S8_UINT      68%    49%     99%
    // VK_FORMAT_D16_UNORM_S8_UINT      31%    54%      1%

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        return VK_FORMAT_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D16_UNORM_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        return VK_FORMAT_D16_UNORM_S8_UINT;
    }

    NS_ASSERT_UNREACHABLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> VKRenderDevice::CreateTexture(const char* label, uint32_t width, uint32_t height,
    uint32_t levels, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memFlags, VkImageAspectFlags aspect)
{
    Ptr<VKTexture> texture = MakePtr<VKTexture>(mLastTextureHashValue++);

    texture->device = this;

    texture->format = format;
    texture->width = width;
    texture->height = height;
    texture->levels = levels;

    const char* suffix = samples > VK_SAMPLE_COUNT_1_BIT ?
        (aspect == VK_IMAGE_ASPECT_COLOR_BIT ? "_AA" : "_Stencil_AA") :
        (aspect == VK_IMAGE_ASPECT_COLOR_BIT ? "" : "_Stencil");

    // Create image
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.samples = samples;
    createInfo.usage = usage;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.mipLevels = levels;
    createInfo.extent.depth = 1;
    createInfo.arrayLayers = 1;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    V(vkCreateImage(mDevice, &createInfo, nullptr, &texture->image));
    VK_NAME(texture->image, IMAGE, "Noesis_%s%s", label, suffix);

    // Allocate memory
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mDevice, texture->image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    V(vkAllocateMemory(mDevice, &allocInfo, nullptr, &texture->memory));
    V(vkBindImageMemory(mDevice, texture->image, texture->memory, 0));
    VK_NAME(texture->memory, DEVICE_MEMORY, "Noesis_%s%s_Mem", label, suffix);
    NS_LOG_TRACE("Texture '%s' created (%zu KB) (Type %02d)", label,
        allocInfo.allocationSize / 1024, allocInfo.memoryTypeIndex);

    // Create View
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.image = texture->image;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.levelCount = levels;
    viewInfo.subresourceRange.layerCount = 1;

    V(vkCreateImageView(mDevice, &viewInfo, nullptr, &texture->view));
    VK_NAME(texture->view, IMAGE_VIEW, "Noesis_%s%s_View", label, suffix);

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VKRenderDevice::Page* VKRenderDevice::AllocatePage(DynamicBuffer& buffer)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer.size;
    bufferInfo.usage = buffer.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Page* page = (Page*)Alloc(sizeof(Page));
    memset(page, 0, sizeof(Page));

    page->hash = mLastBufferHashValue++;

    V(vkCreateBuffer(mDevice, &bufferInfo, nullptr, &page->buffer));
    VK_NAME(page->buffer, BUFFER, "Noesis_%s[%d]", buffer.label, buffer.numPages);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, page->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, buffer.memFlags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    V(vkAllocateMemory(mDevice, &allocInfo, nullptr, &page->memory));
    VK_NAME(page->memory, DEVICE_MEMORY, "Noesis_%s_Mem[%d]", buffer.label, buffer.numPages);

    V(vkBindBufferMemory(mDevice, page->buffer, page->memory, 0));
    V(vkMapMemory(mDevice, page->memory, 0, VK_WHOLE_SIZE, 0, &page->base));

    NS_LOG_TRACE("Page '%s[%d]' created (%d KB) (Type %02d)", buffer.label, buffer.numPages,
        buffer.size / 1024, allocInfo.memoryTypeIndex);

    buffer.numPages++;
    return page;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyPage(Page* page)
{
    vkDestroyBuffer(mDevice, page->buffer, nullptr);
    vkFreeMemory(mDevice, page->memory, nullptr);
    Dealloc(page);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateBuffer(DynamicBuffer& buffer, const char* label, uint32_t size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags)
{
    buffer.size = size;
    buffer.pos = 0;
    buffer.drawPos = 0;
    buffer.label = label;
    buffer.usage = usage;
    buffer.memFlags = memFlags;
    buffer.numPages = 0;
    buffer.pendingPages = nullptr;
    buffer.freePages = nullptr;

    for (uint32_t i = 0; i < PREALLOCATED_DYNAMIC_PAGES; i++)
    {
        Page* page = AllocatePage(buffer);
        page->next = buffer.freePages;
        buffer.freePages = page;
    }

    NS_ASSERT(buffer.freePages != nullptr);
    buffer.currentPage = buffer.freePages;
    buffer.freePages = buffer.freePages->next;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyBuffer(DynamicBuffer& buffer)
{
    if (buffer.currentPage)
    {
        DestroyPage(buffer.currentPage);
    }

    for (Page* page = buffer.pendingPages; page != nullptr;)
    {
        Page* next = page->next;
        DestroyPage(page);
        page = next;
    }

    for (Page* page = buffer.freePages; page != nullptr;)
    {
        Page* next = page->next;
        DestroyPage(page);
        page = next;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKRenderDevice::MapBuffer(DynamicBuffer& buffer, uint32_t size)
{
    NS_ASSERT(size <= buffer.size);

    if (buffer.pos + size > buffer.size)
    {
        // We ran out of space in the current page, get a new one
        // Move the current one to pending and insert a GPU fence
        buffer.currentPage->frameNumber = mFrameNumber;
        buffer.currentPage->next = buffer.pendingPages;
        buffer.pendingPages = buffer.currentPage;

        // If there is one free slot get it
        if (buffer.freePages != nullptr)
        {
            buffer.currentPage = buffer.freePages;
            buffer.freePages = buffer.freePages->next;
        }
        else
        {
            // Move pages already processed by GPU from pending to free
            Page** it = &buffer.pendingPages->next;
            while (*it != nullptr)
            {
                if ((*it)->frameNumber > mSafeFrameNumber)
                {
                    it = &((*it)->next);
                }
                else
                {
                    // Once we find a processed page, the rest of pages must be also processed
                    Page* page = *it;
                    *it = nullptr;

                    NS_ASSERT(buffer.freePages == nullptr);
                    buffer.freePages = page;
                    break;
                }
            }

            if (buffer.freePages != nullptr)
            {
                buffer.currentPage = buffer.freePages;
                buffer.freePages = buffer.freePages->next;
            }
            else
            {
                buffer.currentPage = AllocatePage(buffer);
            }
        }

        buffer.pos = 0;
    }

    buffer.drawPos = buffer.pos;
    buffer.pos = buffer.pos + size;
    return (uint8_t*)buffer.currentPage->base + buffer.drawPos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateBuffers()
{
    memset(mCachedConstantHash, 0, sizeof(mCachedConstantHash));

    CreateBuffer(mVertices, "Vertices", DYNAMIC_VB_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mIndices, "Indices", DYNAMIC_IB_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mConstants[0], "VertexCB0", CBUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mConstants[1], "VertexCB1", CBUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mConstants[2], "PixelCB0", CBUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mConstants[3], "PixelCB1", CBUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateBuffer(mTexUpload, "TexUpload", DYNAMIC_TEX_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyBuffers()
{
    DestroyBuffer(mVertices);
    DestroyBuffer(mIndices);
    DestroyBuffer(mTexUpload);

    for (DynamicBuffer& buffer : mConstants)
    {
        DestroyBuffer(buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateShaders()
{
    uint8_t* shaders = (uint8_t*)Alloc(FastLZ::DecompressBufferSize(Shaders));
    FastLZ::Decompress(Shaders, sizeof(Shaders), shaders);

    for (uint32_t i = 0; i < Shader::Vertex::Count; i++)
    {
        const ShaderVS& vShader = ShadersVS(i, mCaps.linearRendering, false);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = vShader.size;
        createInfo.pCode = (uint32_t*)(shaders + vShader.start);

        V(vkCreateShaderModule(mDevice, &createInfo, nullptr, &mVertexShaders[i]));
        VK_NAME(mVertexShaders[i], SHADER_MODULE, "Noesis_%s", vShader.label);
    }

    if (mStereoSupport)
    {
        for (uint32_t i = 0; i < Shader::Vertex::Count; i++)
        {
            const ShaderVS& vShader = ShadersVS(i, mCaps.linearRendering, true);

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = vShader.size;
            createInfo.pCode = (uint32_t*)(shaders + vShader.start);

            V(vkCreateShaderModule(mDevice, &createInfo, nullptr, &mVertexShadersVR[i]));
            VK_NAME(mVertexShadersVR[i], SHADER_MODULE, "Noesis_Stereo_%s", vShader.label);
        }
    }

    for (uint32_t i = 0; i < Shader::Count; i++)
    {
        const ShaderPS& pShader = ShadersPS(i);
        mPixelShaders[i] = VK_NULL_HANDLE;

        if (pShader.label != nullptr)
        {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = pShader.size;
            createInfo.pCode = (uint32_t*)(shaders + pShader.start);

            V(vkCreateShaderModule(mDevice, &createInfo, nullptr, &mPixelShaders[i]));
            VK_NAME(mPixelShaders[i], SHADER_MODULE, "Noesis_%s", pShader.label);
        }
    }

    Dealloc(shaders);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyShaders()
{
    for (VkShaderModule& shader : mVertexShaders)
    {
        vkDestroyShaderModule(mDevice, shader, nullptr);
    }

    if (mStereoSupport)
    {
        for (VkShaderModule& shader : mVertexShadersVR)
        {
            vkDestroyShaderModule(mDevice, shader, nullptr);
        }
    }

    for (VkShaderModule& shader : mPixelShaders)
    {
        vkDestroyShaderModule(mDevice, shader, nullptr);
    }

    for (CustomShader& shader : mCustomShaders)
    {
        vkDestroyShaderModule(mDevice, shader.module, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void FillLayoutBindings(uint32_t buffersVS, uint32_t buffersPS, uint32_t textures,
    BaseVector<VkDescriptorSetLayoutBinding>& v)
{
    NS_ASSERT(buffersVS + buffersPS <= 4);
    NS_ASSERT(textures <= 3);

    uint32_t binding = 0;

    for (uint32_t i = 0; i < buffersVS; i++)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorCount = 1;
        layoutBinding.binding = binding++;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        v.PushBack(layoutBinding);
    }

    for (uint32_t i = 0; i < buffersPS; i++)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorCount = 1;
        layoutBinding.binding = binding++;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        v.PushBack(layoutBinding);
    }

    for (uint32_t i = 0; i < textures; i++)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.descriptorCount = 1;
        layoutBinding.binding = binding++;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        v.PushBack(layoutBinding);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateLayout(uint32_t signature, Layout& layout)
{
    uint32_t buffersVS = 0;
    uint32_t buffersPS = 0;
    uint32_t textures = 0;

    if (signature & VS_CB0) buffersVS++;
    if (signature & VS_CB1) buffersVS++;
    if (signature & PS_CB0) buffersPS++;
    if (signature & PS_CB1) buffersPS++;
    if (signature & PS_T0) textures++;
    if (signature & PS_T1) textures++;
    if (signature & PS_T2) textures++;
    if (signature & PS_T3) textures++;
    if (signature & PS_T4) textures++;

    uint32_t hash = buffersVS | (buffersPS << 8) | (textures << 16);
    auto r = mLayoutMap.Insert(hash, layout);
    NS_ASSERT(mLayoutMap.IsSmall());

    VkDescriptorSetLayout& setLayout = r.first->value.setLayout;
    VkPipelineLayout& pipelineLayout = r.first->value.pipelineLayout;

    if (r.second)
    {
        // VkDescriptorSetLayout
        Vector<VkDescriptorSetLayoutBinding, 16> layoutBindings;
        FillLayoutBindings(buffersVS, buffersPS, textures, layoutBindings);
        NS_ASSERT(layoutBindings.IsSmall());

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = layoutBindings.Size();
        layoutInfo.pBindings = layoutBindings.Data();

        V(vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &setLayout));
        VK_NAME(setLayout, DESCRIPTOR_SET_LAYOUT, "Noesis_SetLayout_B%d_B%d_T%d",
            buffersVS, buffersPS, textures);

        // VkPipelineLayout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &setLayout;

        V(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));
        VK_NAME(pipelineLayout, PIPELINE_LAYOUT, "Noesis_PipelineLayout_B%d_B%d_T%d",
            buffersVS, buffersPS, textures);
    }

    layout.setLayout = setLayout;
    layout.pipelineLayout = pipelineLayout;
    layout.signature = signature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateLayouts()
{
    for (uint32_t i = 0; i < Shader::Count; i++)
    {
        const ShaderPS& pShader = ShadersPS(i);
        CreateLayout(pShader.signature, mLayouts[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyLayouts()
{
    for (auto& layout : mLayoutMap)
    {
        vkDestroyDescriptorSetLayout(mDevice, layout.value.setLayout, nullptr);
        vkDestroyPipelineLayout(mDevice, layout.value.pipelineLayout, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateTransferCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = mQueueFamilyIndex;

    V(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mTransferCommandPool));
    VK_NAME(mTransferCommandPool, COMMAND_POOL, "%s", "Noesis_Transfer_CommandPool");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyTransferCommandPool()
{
    vkDestroyCommandPool(mDevice, mTransferCommandPool, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateDescriptorPool()
{
    if (NS_LIKELY(mDescriptorPool))
    {
        // Current pool is exhausted
        mFreeDescriptorPools.PushBack(MakePair(mDescriptorPool, mFrameNumber));

        // If possible, recycle a previously created pool
        if (mFreeDescriptorPools.Size() > 1)
        {
            if (mFreeDescriptorPools.Front().second <= mSafeFrameNumber)
            {
                mDescriptorPool = mFreeDescriptorPools.Front().first;
                V(vkResetDescriptorPool(mDevice, mDescriptorPool, 0));
                mFreeDescriptorPools.Erase(mFreeDescriptorPools.Begin());
                return;
            }
        }
    }

    VkDescriptorPoolSize poolSizes[2] =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DESCRIPTOR_POOL_MAX_SETS * 4 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTOR_POOL_MAX_SETS * 3 }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = DESCRIPTOR_POOL_MAX_SETS;

    V(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool));
    VK_NAME(mDescriptorPool, DESCRIPTOR_POOL, "Noesis_Pool_%d", mFreeDescriptorPools.Size());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyDescriptorPool()
{
    vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);

    for (auto& v : mFreeDescriptorPools)
    {
        vkDestroyDescriptorPool(mDevice, v.first, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetMinMagFilter(MinMagFilter::Enum minmag, VkSamplerCreateInfo& samplerInfo)
{
    switch (minmag)
    {
        case MinMagFilter::Nearest:
        {
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            break;
        }
        case MinMagFilter::Linear:
        {
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetMipFilter(MipFilter::Enum mip, VkSamplerCreateInfo& samplerInfo)
{
    switch (mip)
    {
        case MipFilter::Disabled:
        {
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.maxLod = 0.0f;
            break;
        }
        case MipFilter::Nearest:
        {
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.maxLod = FLT_MAX;
            break;
        }
        case MipFilter::Linear:
        {
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.maxLod = FLT_MAX;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetAddress(WrapMode::Enum mode, VkSamplerCreateInfo& samplerInfo)
{
    switch (mode)
    {
        case WrapMode::ClampToEdge:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        }
        case WrapMode::ClampToZero:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
        }
        case WrapMode::Repeat:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        }
        case WrapMode::MirrorU:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        }
        case WrapMode::MirrorV:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        }
        case WrapMode::Mirror:
        {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreateSamplers()
{
    memset(mSamplers, 0, sizeof(mSamplers));

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.mipLodBias = -0.75f;

    const char* MinMagStr[] = { "Nearest", "Linear" };
    const char* MipStr[] = { "Disabled", "Nearest", "Linear" };
    const char* WrapStr[] = { "ClampToEdge", "ClampToZero", "Repeat", "MirrorU", "MirrorV", "Mirror" };

    for (uint8_t minmag = 0; minmag < MinMagFilter::Count; minmag++)
    {
        for (uint8_t mip = 0; mip < MipFilter::Count; mip++)
        {
            SetMinMagFilter(MinMagFilter::Enum(minmag), samplerInfo);
            SetMipFilter(MipFilter::Enum(mip), samplerInfo);

            for (uint8_t uv = 0; uv < WrapMode::Count; uv++)
            {
                SetAddress(WrapMode::Enum(uv), samplerInfo);

                SamplerState s = {{ uv, minmag, mip }};
                V(vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSamplers[s.v]));
                VK_NAME(mSamplers[s.v], SAMPLER, "Noesis_%s_%s_%s", MinMagStr[minmag], MipStr[mip],
                    WrapStr[uv]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroySamplers()
{
    for (VkSampler sampler : mSamplers)
    {
        vkDestroySampler(mDevice, sampler, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VkRenderPass VKRenderDevice::CreateRenderPass(VkSampleCountFlagBits samples, bool needsStencil)
{
    Vector<VkAttachmentDescription, 3> attachments;

    VkAttachmentReference stencilRef{};
    VkAttachmentReference colorRef{};
    VkAttachmentReference resolveRef{};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;

    VkSubpassDependency dependencies[2]{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    if (needsStencil)
    {
        // Stencil: Don't care -> Don't care
        VkAttachmentDescription stencil{};
        stencil.samples = samples;
        stencil.format = mStencilFormat;
        stencil.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        stencil.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        stencil.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        stencil.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        stencil.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        stencil.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        stencilRef.attachment = attachments.Size();
        stencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &stencilRef;
        attachments.PushBack(stencil);
    }

    if (samples > 1)
    {
      #if RESOLVE_ON_WRITEBACK
        // Multisampled Color: Don't care -> Don't care
        VkAttachmentDescription color{};
        color.format = mBackBufferFormat;
        color.samples = samples;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorRef.attachment = attachments.Size();
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.pColorAttachments = &colorRef;
        attachments.PushBack(color);

        // Resolved Color: Don't care -> Store
        VkAttachmentDescription resolve{};
        resolve.format = mBackBufferFormat;
        resolve.samples = VK_SAMPLE_COUNT_1_BIT;
        resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolve.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        resolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        resolveRef.attachment = attachments.Size();
        resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.pResolveAttachments = &resolveRef;
        attachments.PushBack(resolve);

      #else
        // Multisampled Color: Don't care -> Store
        VkAttachmentDescription color{};
        color.format = mBackBufferFormat;
        color.samples = samples;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        color.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        colorRef.attachment = attachments.Size();
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.pColorAttachments = &colorRef;
        attachments.PushBack(color);

        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      #endif
    }
    else
    {
        // Color: Don't care -> Store
        VkAttachmentDescription color{};
        color.format = mBackBufferFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        colorRef.attachment = attachments.Size();
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.pColorAttachments = &colorRef;
        attachments.PushBack(color);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.attachmentCount = attachments.Size();
    renderPassInfo.pAttachments = attachments.Data();
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    VkRenderPass renderPass;
    V(vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &renderPass));
    VK_NAME(renderPass, RENDER_PASS, samples > 1 ? "Noesis_RGBA8%s_x%d_RenderPass" :
        "Noesis_RGBA8%s_RenderPass", needsStencil ? "_S8" : "", samples);
    return renderPass;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyRenderPasses()
{
    for (uint32_t i = 0; i < NS_COUNTOF(mRenderPasses); i++)
    {
        vkDestroyRenderPass(mDevice, mRenderPasses[i][0], nullptr);
        vkDestroyRenderPass(mDevice, mRenderPasses[i][1], nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SetStencilRef(uint32_t stencilRef)
{
    if (mCachedStencilRef != stencilRef)
    {
        vkCmdSetStencilReference(mCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilRef);
        mCachedStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t HashPipeline(VkRenderPass pass, uint8_t id, uint8_t state, bool stereo, uint32_t custom)
{
    return HashCombine(pass, custom, uint32_t((id << 16) | (state << 8) | (stereo ? 1 : 0)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::BindPipeline(const Batch& batch)
{
    uint32_t hash = HashPipeline(mActiveRenderPass, batch.shader.v, batch.renderState.v,
        batch.singlePassStereo, (int)(uintptr_t)batch.pixelShader);
    VkPipeline pipeline = mPipelines[mPipelineMap.Find(hash)->value];

    if (pipeline != mCachedPipeline)
    {
        vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        mCachedPipeline = pipeline;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::SetBuffers(const Batch& batch)
{
    VkBuffer buffer = mVertices.currentPage->buffer;
    VkDeviceSize offset = mVertices.drawPos + batch.vertexOffset;
    vkCmdBindVertexBuffers(mCommandBuffer, 0, 1, &buffer, &offset);

    if (mCachedIndexBuffer != mIndices.currentPage->buffer)
    {
        mCachedIndexBuffer = mIndices.currentPage->buffer;
        vkCmdBindIndexBuffer(mCommandBuffer, mCachedIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::UploadUniforms(uint32_t i, const UniformData* data, uint32_t& hash,
    BaseVector<uint32_t>& offsets)
{
    NS_ASSERT(data != 0);
    NS_ASSERT(data->numDwords > 0);

    if (mCachedConstantHash[i] != data->hash)
    {
        VkDeviceSize alignment = mDeviceProperties.limits.minUniformBufferOffsetAlignment;
        uint32_t size = data->numDwords * sizeof(uint32_t);
        void* ptr = MapBuffer(mConstants[i], Align(size, (uint32_t)alignment));
        memcpy(ptr, data->values, size);

        mCachedConstantHash[i] = data->hash;
    }

    hash = Hash(hash, mConstants[i].currentPage->hash | (data->numDwords << 16));
    offsets.PushBack(mConstants[i].drawPos);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::FillBufferInfo(uint32_t i, const UniformData* data, VkDescriptorSet set,
    BaseVector<VkDescriptorBufferInfo>& buffers, BaseVector<VkWriteDescriptorSet>& writes,
    uint32_t& binding)
{
    NS_ASSERT(data != 0);
    NS_ASSERT(data->numDwords > 0);

    VkDescriptorBufferInfo& info = buffers.EmplaceBack();
    info.buffer = mConstants[i].currentPage->buffer;
    info.range = data->numDwords * sizeof(uint32_t);

    VkWriteDescriptorSet& write = writes.EmplaceBack();
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding++;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.descriptorCount = 1;
    write.pBufferInfo = &info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::FillImageInfo(Texture* texture, uint8_t sampler, VkDescriptorSet set,
    BaseVector<VkDescriptorImageInfo>& images, BaseVector<VkWriteDescriptorSet>& writes,
    uint32_t& binding)
{
    NS_ASSERT(texture != 0);

    VkDescriptorImageInfo& info = images.EmplaceBack();
    info.sampler = mSamplers[sampler];
    info.imageView = ((VKTexture*)texture)->view;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet& write = writes.EmplaceBack();
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding++;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::TextureHash(uint32_t& hash, Texture* texture_, uint8_t sampler)
{
    NS_ASSERT(texture_ != 0);
    VKTexture* texture = (VKTexture*)texture_;

    if (NS_UNLIKELY(texture->hash == 0))
    {
        // This is the first use of a wrapped texture
        texture->hash = mLastTextureHashValue++;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = texture->format;
        viewInfo.image = texture->image;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = texture->levels;
        viewInfo.subresourceRange.layerCount = 1;

        V(vkCreateImageView(mDevice, &viewInfo, nullptr, &texture->view));
    }

    hash = Hash(hash, texture->hash | sampler << 24);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::BindDescriptors(const Batch& batch, const Layout& layout)
{
    uint32_t signature = layout.signature;

    VKTexture* pattern = (VKTexture*)batch.pattern;
    VKTexture* ramps = (VKTexture*)batch.ramps;
    VKTexture* image = (VKTexture*)batch.image;
    VKTexture* glyphs = (VKTexture*)batch.glyphs;
    VKTexture* shadow = (VKTexture*)batch.shadow;

    uint8_t patternSampler = batch.patternSampler.v;
    uint8_t rampsSampler = batch.rampsSampler.v;
    uint8_t imageSampler = batch.imageSampler.v;
    uint8_t glyphsSampler = batch.glyphsSampler.v;
    uint8_t shadowSampler = batch.shadowSampler.v;

    const UniformData* vsCB0 = &batch.vertexUniforms[0];
    const UniformData* vsCB1 = &batch.vertexUniforms[1];
    const UniformData* psCB0 = &batch.pixelUniforms[0];
    const UniformData* psCB1 = &batch.pixelUniforms[1];

    // Although a descriptor pool without VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT should
    // allocate very fast it is recommended to optimize calls to vkAllocateDescriptorSets() for
    // mobiles. The strategy here is hashing each descriptor set for reusing

    uint32_t hash = 0x050c5d1f;

    Vector<uint32_t, 4> offsets;
    if (signature & VS_CB0) UploadUniforms(0, vsCB0, hash, offsets);
    if (signature & VS_CB1) UploadUniforms(1, vsCB1, hash, offsets);
    if (signature & PS_CB0) UploadUniforms(2, psCB0, hash, offsets);
    if (signature & PS_CB1) UploadUniforms(3, psCB1, hash, offsets);

    if (signature & PS_T0) TextureHash(hash, pattern, patternSampler);
    if (signature & PS_T1) TextureHash(hash, ramps, rampsSampler);
    if (signature & PS_T2) TextureHash(hash, image, imageSampler);
    if (signature & PS_T3) TextureHash(hash, glyphs, glyphsSampler);
    if (signature & PS_T4) TextureHash(hash, shadow, shadowSampler);

    auto it = mDescriptorSetMap.Find(hash);

    if (NS_UNLIKELY(it == mDescriptorSetMap.End()))
    {
        if (NS_UNLIKELY(mDescriptorSetMap.Size() == DESCRIPTOR_POOL_MAX_SETS))
        {
            // Current pools is exhausted, get a new one and clear the cache
            mDescriptorSetMap.Clear();
            CreateDescriptorPool();
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout.setLayout;

        VkDescriptorSet set;
        V(vkAllocateDescriptorSets(mDevice, &allocInfo, &set));
        VK_NAME(set, DESCRIPTOR_SET, "Noesis_Set_%d", mDescriptorSetMap.Size());
        NS_LOG_TRACE("DescriptorSet 'Set_%d' created", mDescriptorSetMap.Size());

        it = mDescriptorSetMap.Insert(hash, set).first;

        Vector<VkDescriptorBufferInfo, 4> buffers;
        Vector<VkDescriptorImageInfo, 5> images;
        Vector<VkWriteDescriptorSet, 9> writes;

        uint32_t binding = 0;

        if (signature & VS_CB0) FillBufferInfo(0, vsCB0, set, buffers, writes, binding);
        if (signature & VS_CB1) FillBufferInfo(1, vsCB1, set, buffers, writes, binding);
        if (signature & PS_CB0) FillBufferInfo(2, psCB0, set, buffers, writes, binding);
        if (signature & PS_CB1) FillBufferInfo(3, psCB1, set, buffers, writes, binding);

        if (signature & PS_T0) FillImageInfo(pattern, patternSampler, set, images, writes, binding);
        if (signature & PS_T1) FillImageInfo(ramps, rampsSampler, set, images, writes, binding);
        if (signature & PS_T2) FillImageInfo(image, imageSampler, set, images, writes, binding);
        if (signature & PS_T3) FillImageInfo(glyphs, glyphsSampler, set, images, writes, binding);
        if (signature & PS_T4) FillImageInfo(shadow, shadowSampler, set, images, writes, binding);

        NS_ASSERT(buffers.IsSmall());
        NS_ASSERT(images.IsSmall());
        NS_ASSERT(writes.IsSmall());

        vkUpdateDescriptorSets(mDevice, writes.Size(), writes.Data(), 0, nullptr);
    }

    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout.pipelineLayout,
        0, 1, &it->value, offsets.Size(), offsets.Data());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static VkFormat Format(uint32_t type)
{
    switch (type)
    {
        case Shader::Vertex::Format::Attr::Type::Float: return VK_FORMAT_R32_SFLOAT;
        case Shader::Vertex::Format::Attr::Type::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case Shader::Vertex::Format::Attr::Type::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Shader::Vertex::Format::Attr::Type::UByte4Norm: return VK_FORMAT_R8G8B8A8_UNORM;
        case Shader::Vertex::Format::Attr::Type::UShort4Norm: return VK_FORMAT_R16G16B16A16_UNORM;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void FillVertexAttributes(uint32_t format, BaseVector<VkVertexInputAttributeDescription>& v)
{
    uint32_t attributes = AttributesForFormat[format];
    uint32_t offset = 0;

    for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
    {
        if (attributes & (1 << i))
        {
            VkVertexInputAttributeDescription& attr = v.EmplaceBack();
            memset(&attr, 0, sizeof(attr));

            attr.binding = 0;
            attr.location = i;
            attr.format = Format(TypeForAttr[i]);
            attr.offset = offset;

            offset += SizeForType[TypeForAttr[i]];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void RasterizerInfo(VkPipelineRasterizationStateCreateInfo& info, RenderState state,
    const VkPhysicalDeviceFeatures& features, BaseString& label)
{
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.lineWidth = 1.0f;
    info.cullMode = VK_CULL_MODE_NONE;
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;

    if (state.f.wireframe && features.fillModeNonSolid)
    {
        label += "_Wire";
        info.polygonMode = VK_POLYGON_MODE_LINE;
    }
    else
    {
        info.polygonMode = VK_POLYGON_MODE_FILL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void BlendInfo(VkPipelineColorBlendAttachmentState& info, RenderState state, BaseString& label)
{
    if (state.f.colorEnable)
    {
        info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        info.colorBlendOp = VK_BLEND_OP_ADD;
        info.alphaBlendOp = VK_BLEND_OP_ADD;

        switch (state.f.blendMode)
        {
            case BlendMode::Src:
            {
                info.blendEnable = VK_FALSE;
                break;
            }
            case BlendMode::SrcOver:
            {
                label += "_SrcOver";
                info.blendEnable = VK_TRUE;
                info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Multiply:
            {
                label += "_SrcOver_Multiply";
                info.blendEnable = VK_TRUE;
                info.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
                info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Screen:
            {
                label += "_SrcOver_Screen";
                info.blendEnable = VK_TRUE;
                info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Additive:
            {
                label += "_SrcOver_Additive";
                info.blendEnable = VK_TRUE;
                info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                break;
            }
            default:
            {
                NS_ASSERT_UNREACHABLE;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void DepthStencilInfo(VkPipelineDepthStencilStateCreateInfo& info, RenderState state,
    BaseString& label)
{
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthWriteEnable = VK_FALSE;
    info.depthBoundsTestEnable = VK_FALSE;

    info.front.failOp = VK_STENCIL_OP_KEEP;
    info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    info.front.writeMask = 0xFF;
    info.front.compareMask = 0XFF;

    info.back.failOp = VK_STENCIL_OP_KEEP;
    info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    info.back.writeMask = 0xFF;
    info.back.compareMask = 0XFF;

    switch (state.f.stencilMode)
    {
        case StencilMode::Disabled:
        {
            info.depthTestEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_NEVER;
            info.stencilTestEnable = VK_FALSE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_KEEP;
            info.back.passOp = VK_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Keep:
        {
            label += "_Equal_Keep";
            info.depthTestEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_NEVER;
            info.stencilTestEnable = VK_TRUE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_KEEP;
            info.back.passOp = VK_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Incr:
        {
            label += "_Equal_Incr";
            info.depthTestEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_NEVER;
            info.stencilTestEnable = VK_TRUE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
            info.back.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
            break;
        }
        case StencilMode::Equal_Decr:
        {
            label += "_Equal_Decr";
            info.depthTestEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_NEVER;
            info.stencilTestEnable = VK_TRUE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
            info.back.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
            break;
        }
        case StencilMode::Clear:
        {
            label += "_Clear";
            info.depthTestEnable = VK_FALSE;
            info.depthCompareOp = VK_COMPARE_OP_NEVER;
            info.stencilTestEnable = VK_TRUE;
            info.front.compareOp = VK_COMPARE_OP_ALWAYS;
            info.back.compareOp = VK_COMPARE_OP_ALWAYS;
            info.front.passOp = VK_STENCIL_OP_ZERO;
            info.back.passOp = VK_STENCIL_OP_ZERO;
            break;
        }
        case StencilMode::Disabled_ZTest:
        {
            label += "_ZTest";
            info.depthTestEnable = VK_TRUE;
            info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            info.stencilTestEnable = VK_FALSE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_KEEP;
            info.back.passOp = VK_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Keep_ZTest:
        {
            label += "_Equal_Keep_ZTest";
            info.depthTestEnable = VK_TRUE;
            info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            info.stencilTestEnable = VK_TRUE;
            info.front.compareOp = VK_COMPARE_OP_EQUAL;
            info.back.compareOp = VK_COMPARE_OP_EQUAL;
            info.front.passOp = VK_STENCIL_OP_KEEP;
            info.back.passOp = VK_STENCIL_OP_KEEP;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreatePipelines(const char* label_, uint8_t shader_, VkRenderPass renderPass,
    VkGraphicsPipelineCreateInfo& pipelineInfo, bool stereo, uint32_t custom)
{
    VkPipeline parent = VK_NULL_HANDLE;

    for (uint32_t i = 0; i < 256; i++)
    {
        Shader shader;
        shader.v = shader_;

        RenderState state;
        state.v = (uint8_t)i;

        if (IsValidState(shader, state))
        {
            pipelineInfo.basePipelineHandle = parent;
            pipelineInfo.flags = (parent != VK_NULL_HANDLE) ? VK_PIPELINE_CREATE_DERIVATIVE_BIT :
                VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

            FixedString<256> label = label_;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            RasterizerInfo(rasterizer, state, mDeviceFeatures, label);

            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            DepthStencilInfo(depthStencil, state, label);

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            BlendInfo(colorBlendAttachment, state, label);

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;

            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;

            VkPipeline pipeline;
            V(vkCreateGraphicsPipelines(mDevice, mPipelineCache, 1, &pipelineInfo, 0, &pipeline));
            VK_NAME(pipeline, PIPELINE, "Noesis_%s", label.Str());

            parent = (parent == VK_NULL_HANDLE) ? pipeline : parent;

            uint32_t hash = HashPipeline(renderPass, shader_, (uint8_t)i, stereo, custom);
            mPipelineMap.Insert(hash, mPipelines.Size());
            mPipelines.PushBack(pipeline);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreatePipelines(const char* label, uint8_t shader, VkRenderPass renderPass,
    VkShaderModule psModule, VkPipelineLayout layout, VkSampleCountFlagBits sampleCount,
    uint32_t custom)
{
    uint8_t vsIndex = VertexForShader[shader];

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.layout = layout;

    // Vertex Input State
    uint32_t format = FormatForVertex[vsIndex];
    Vector<VkVertexInputAttributeDescription, Shader::Vertex::Format::Attr::Count> attrs;
    FillVertexAttributes(format, attrs);

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = SizeForFormat[format];
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attrs.Size();
    vertexInputInfo.pVertexAttributeDescriptions = attrs.Data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    // Shader Stages
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = mVertexShaders[vsIndex];
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = psModule;
    shaderStages[1].pName = "main";

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // Multisample State
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sampleCount;
    multisampling.minSampleShading = 1.0f;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    pipelineInfo.pMultisampleState = &multisampling;

    // Dynamic State
    VkDynamicState dynamicStates[] =
    { 
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = NS_COUNTOF(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    pipelineInfo.pDynamicState = &dynamicState;

    // Viewport State
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    pipelineInfo.pViewportState = &viewportState;

    // Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    pipelineInfo.pInputAssemblyState = &inputAssembly;

    CreatePipelines(label, shader, renderPass, pipelineInfo, false, custom);

    if (mStereoSupport)
    {
        shaderStages[0].module = mVertexShadersVR[vsIndex];
        CreatePipelines(label, shader, renderPass, pipelineInfo, true, custom);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::CreatePipelines(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount)
{
    auto r = mCachedPipelineRenderPasses.Insert(renderPass, sampleCount);

    if (NS_LIKELY(!r.second))
    {
        return;
    }

  #ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
  #endif

    for (uint32_t i = 0; i < Shader::Count; i++)
    {
        const ShaderPS& pShader = ShadersPS(i);

        if (pShader.label != nullptr)
        {
            CreatePipelines(pShader.label, (uint8_t)i, renderPass, mPixelShaders[i],
                mLayouts[i].pipelineLayout, sampleCount, 0);
        }
    }

    for (uint32_t i = 0; i < mCustomShaders.Size(); i++)
    {
        const CustomShader& customShader = mCustomShaders[i];
        CreatePipelines(customShader.label.Str(), customShader.id, renderPass,
            customShader.module, customShader.layout.pipelineLayout, sampleCount, i + 1);
    }

  #ifdef NS_PROFILE
    uint64_t t1 = HighResTimer::Ticks();
    NS_LOG_TRACE("Pipelines compiled in %.0f ms [0x%" PRIX64 "]",
        HighResTimer::Milliseconds(t1 - t0), (uint64_t)renderPass);
   #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::DestroyPipelines()
{
    for (VkPipeline pipeline : mPipelines)
    {
        vkDestroyPipeline(mDevice, pipeline, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::ProcessPendingDestroys(bool force)
{
    if (NS_UNLIKELY(!mPendingDestroys.Empty()))
    {
        PendingDestroy* it = mPendingDestroys.Begin();
        while (it != mPendingDestroys.End())
        {
            if (it->frame <= mSafeFrameNumber || force)
            {
                if (it->buffer)
                {
                    vkDestroyBuffer(mDevice, it->buffer, nullptr);
                }

                if (it->image)
                {
                    vkDestroyImage(mDevice, it->image, nullptr);
                }

                if (it->memory)
                {
                    vkFreeMemory(mDevice, it->memory, nullptr);
                }

                if (it->view)
                {
                    vkDestroyImageView(mDevice, it->view, nullptr);
                }

                if (it->framebuffer)
                {
                    vkDestroyFramebuffer(mDevice, it->framebuffer, nullptr);
                }

                it++;
            }
            else
            {
                break;
            }
        }

        mPendingDestroys.Erase(mPendingDestroys.Begin(), it);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::EnsureTransferCommands()
{
    if (mTransferCommands == VK_NULL_HANDLE)
    {
        if (mTransferPoolFrameNumber <= mSafeFrameNumber)
        {
            V(vkResetCommandPool(mDevice, mTransferCommandPool, 0));
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = mTransferCommandPool;
        allocInfo.commandBufferCount = 1;

        V(vkAllocateCommandBuffers(mDevice, &allocInfo, &mTransferCommands));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        V(vkBeginCommandBuffer(mTransferCommands, &beginInfo));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::FlushTransferCommands()
{
    if (NS_UNLIKELY(mTransferCommands != VK_NULL_HANDLE))
    {
        for (Texture* texture : mUpdatedTextures)
        {
            ChangeLayout(mTransferCommands, texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        V(vkEndCommandBuffer(mTransferCommands));
        mQueueSubmit(mTransferCommands);

        mUpdatedTextures.Clear();
        mTransferCommands = nullptr;
        mTransferPoolFrameNumber = mFrameNumber;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderDevice::ChangeLayout(VkCommandBuffer commands, Texture* texture_, VkImageLayout layout)
{
    VKTexture* texture = (VKTexture*)texture_;

    if (texture->layout != layout)
    {
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkAccessFlags srcAccessMask = 0;
        VkAccessFlags dstAccessMask = 0;

        switch (texture->layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
            {
                break;
            }
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            {
                srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            }
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            {
                srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            }
            default:
                NS_ASSERT_UNREACHABLE;
        }

        switch (layout)
        {
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            {
                dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            }
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            {
                dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;
            }
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            {
                dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            }
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            {
                dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            }
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            {
                dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            }
            default:
                NS_ASSERT_UNREACHABLE;
        }

        VkImageAspectFlags aspectMask;

        switch (texture->format)
        {
            case VK_FORMAT_S8_UINT:
            {
                aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                break;
            }
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            {
                aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
                break;
            }
            default:
            {
                aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = texture->image;
        barrier.oldLayout = texture->layout;
        barrier.newLayout = layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.levelCount = texture->levels;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(commands, srcStageMask, dstStageMask, 0, 0, 0, 0, 0, 1, &barrier);

        texture->layout = layout;
    }
}
