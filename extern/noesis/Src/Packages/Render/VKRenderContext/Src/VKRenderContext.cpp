////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "VKRenderContext.h"

#include <NsCore/Ptr.h>
#include <NsCore/Log.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/Category.h>
#include <NsCore/HighResTimer.h>
#include <NsDrawing/Color.h>
#include <NsRender/Image.h>
#include <NsRender/RenderDevice.h>
#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>
#include <NsRender/VKFactory.h>

#include <inttypes.h>

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#include <Shlobj.h>
#define LOAD_FUNC(x) x = (decltype(x))GetProcAddress((HMODULE)mVulkanLib, #x)
#endif

#ifdef NS_PLATFORM_ANDROID
#include <dlfcn.h>
#include <NsApp/Display.h>
#include <android_native_app_glue.h>
#define LOAD_FUNC(x) x = (decltype(x))dlsym(mVulkanLib, #x)
#endif


using namespace Noesis;
using namespace NoesisApp;


#define V(exp) \
    NS_MACRO_BEGIN \
        VkResult err_ = (exp); \
        NS_ASSERT(err_ == VK_SUCCESS); \
    NS_MACRO_END

#define LOAD_GLOBAL_FUNC(x) \
    NS_MACRO_BEGIN \
        x = (decltype(x))vkGetInstanceProcAddr(nullptr, #x); \
        NS_ASSERT(x != nullptr); \
    NS_MACRO_END

#define LOAD_INSTANCE_FUNC(x) \
    NS_MACRO_BEGIN \
        NS_ASSERT(mInstance != nullptr); \
        x = (decltype(x))vkGetInstanceProcAddr(mInstance, #x); \
        NS_ASSERT(x != nullptr); \
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
                vkCmdDebugMarkerBeginEXT(mCommandBuffers[mFrameIndex], &info); \
            } \
        NS_MACRO_END
    #define VK_END_EVENT() \
        NS_MACRO_BEGIN \
            if (vkCmdDebugMarkerEndEXT != nullptr) \
            { \
                vkCmdDebugMarkerEndEXT(mCommandBuffers[mFrameIndex]); \
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
        NS_MACRO_END
#else
    #define VK_BEGIN_EVENT(...) NS_NOOP
    #define VK_END_EVENT() NS_NOOP
    #define VK_NAME(obj, type, ...) NS_UNUSED(__VA_ARGS__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
VKRenderContext::VKRenderContext()
{
  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    mVulkanLib = LoadLibraryW(L"vulkan-1.dll");
  #endif

  #ifdef NS_PLATFORM_ANDROID
    mVulkanLib = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
  #endif

    if (mVulkanLib != 0)
    {
        LOAD_FUNC(vkGetInstanceProcAddr);

        if (vkGetInstanceProcAddr != 0)
        {
            LOAD_GLOBAL_FUNC(vkCreateInstance);
            LOAD_GLOBAL_FUNC(vkEnumerateInstanceExtensionProperties);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VKRenderContext::~VKRenderContext()
{
    if (mVulkanLib != 0)
    {
      #ifdef NS_PLATFORM_WINDOWS_DESKTOP
        FreeLibrary((HMODULE)mVulkanLib);
      #endif

      #ifdef NS_PLATFORM_ANDROID
        dlclose(mVulkanLib);
      #endif
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* VKRenderContext::Description() const
{
    return "Vulkan";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* VKRenderContext::ShaderLang() const
{
    return "spirv";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t VKRenderContext::Score() const
{
    return 250;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool VKRenderContext::Validate() const
{
    return mVulkanLib != 0 && vkCreateInstance != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::Init(void* window, uint32_t& samples, bool vSync, bool sRGB)
{
    NS_ASSERT(Validate());
    NS_LOG_DEBUG("Creating Vulkan render context");

    CreateInstance();
    CreateSurface(window);
    CreateDevice(samples, sRGB, vSync);
    CreateRenderPass();
    CreateCommands();
    CreateSyncObjects();
    CreateQueries();
    CreatePipelineCache();

    VKFactory::InstanceInfo info{};
    info.instance = mInstance;
    info.physicalDevice = mPhysicalDevice;
    info.device = mDevice;
    info.pipelineCache = mPipelineCache;
    info.queueFamilyIndex = mQueueFamilyIndex;
    info.vkGetInstanceProcAddr = vkGetInstanceProcAddr;

    info.QueueSubmit = [this](VkCommandBuffer commandBuffer)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        V(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    };

    mRenderer = VKFactory::CreateDevice(sRGB, info);
    VKFactory::WarmUpRenderPass(mRenderer, mRenderPass, mSampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::Shutdown()
{
    V(vkDeviceWaitIdle(mDevice));

    mRenderer.Reset();

    DestroyPipelineCache();
    DestroyQueries();
    DestroySyncObjects();
    DestroyCommands();
    DestroyImageViews();
    DestroySwapChain();
    DestroyRenderPass();
    DestroyDevice();
    DestroySurface();
    DestroyInstance();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::SetWindow(void* window)
{
    DestroySwapChain();
    DestroySurface();
    DestroyImageViews();

    if (window != 0)
    {
        CreateSurface(window);
        CreateSwapChain();
        CreateImageViews();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::SaveState()
{
    SavePipelineCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
RenderDevice* VKRenderContext::GetDevice() const
{
    return mRenderer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::BeginRender()
{
    V(vkWaitForFences(mDevice, 1, &mInFlightFences[mFrameIndex], VK_TRUE, UINT64_MAX));

  #ifdef NS_PROFILE
    if (mHasTimeStampQueries && mFrameNumber >= FramesInFlight)
    {
        uint64_t data[4] = { 0, 0, 0, 0 };
        VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT;
        VkResult r = vkGetQueryPoolResults(mDevice, mTimeStampQueries[mFrameIndex], 0, 2,
            sizeof(data), data, sizeof(data) / 2, flags);

        if (r == VK_SUCCESS)
        {
            NS_ASSERT(data[1] && data[3]);
            mGPUTimeMs = (mTimeStampPeriod * (data[2] - data[0])) / 1000000.0f;
        }
    }
  #endif

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    V(vkResetCommandPool(mDevice, mCommandPools[mFrameIndex], 0));
    V(vkBeginCommandBuffer(mCommandBuffers[mFrameIndex], &beginInfo));

  #ifdef NS_PROFILE
    if (mHasTimeStampQueries)
    {
        VkCommandBuffer commandBuffer = mCommandBuffers[mFrameIndex];
        VkQueryPool queryPool = mTimeStampQueries[mFrameIndex];

        vkCmdResetQueryPool(commandBuffer, queryPool, 0, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 0);
    }
  #endif

    VKFactory::RecordingInfo recordingInfo{};
    recordingInfo.commandBuffer = mCommandBuffers[mFrameIndex];
    recordingInfo.frameNumber = mFrameNumber + FramesInFlight;
    recordingInfo.safeFrameNumber = mFrameNumber;

    VKFactory::SetCommandBuffer(mRenderer, recordingInfo);

    VK_BEGIN_EVENT("BeginRender");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::EndRender()
{
    mFrameNumber++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::Resize()
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    V(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &surfaceCaps));

    if (surfaceCaps.currentExtent.width != 0 && surfaceCaps.currentExtent.height != 0)
    {
        V(vkDeviceWaitIdle(mDevice));

        DestroyImageViews();
        CreateSwapChain();
        CreateImageViews();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float VKRenderContext::GetGPUTimeMs() const
{
    return mGPUTimeMs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::SetClearColor(float r, float g, float b, float a)
{
    bool sRGB = mBackBufferFormat == VK_FORMAT_B8G8R8A8_SRGB || 
                mBackBufferFormat == VK_FORMAT_R8G8B8A8_SRGB;

    mClearValues[1].color.float32[0] = sRGB ? SRGBToLinear(r) : r;
    mClearValues[1].color.float32[1] = sRGB ? SRGBToLinear(g) : g;
    mClearValues[1].color.float32[2] = sRGB ? SRGBToLinear(b) : b;
    mClearValues[1].color.float32[3] = a;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::SetDefaultRenderTarget(uint32_t, uint32_t, bool doClearColor)
{
    VK_BEGIN_EVENT("MainRenderTarget");

    VkSemaphore sem = mImageAvailableSemaphores[mFrameIndex];
    VkResult r = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, sem, 0, &mImageIndex);

    if (r != VK_SUCCESS)
    {
        if (r == VK_ERROR_SURFACE_LOST_KHR)
        {
            DestroySwapChain();
            DestroySurface();
            CreateSurface(mWindow);
        }

        Resize();

        r = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, sem, 0, &mImageIndex);
    }

    mImageAcquired = (r == VK_SUCCESS);
    NS_ASSERT(mImageIndex < NS_COUNTOF(mSwapChainFrameBuffers));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = doClearColor ? mRenderPass : mRenderPassNoClear;
    renderPassInfo.framebuffer = mSwapChainFrameBuffers[mImageIndex];
    renderPassInfo.renderArea.extent = mSwapChainExtent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = mClearValues;

    vkCmdBeginRenderPass(mCommandBuffers[mFrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(mCommandBuffers[mFrameIndex], 0, 1, &mViewport);
    vkCmdSetScissor(mCommandBuffers[mFrameIndex], 0, 1, &mScissorRect);

    // Render passes mRenderPass and mRenderPassNoClear are compatible
    // Pass always mRenderPass to avoid creating unnecessary pipelines
    VKFactory::SetRenderPass(mRenderer, mRenderPass, mSampleCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<NoesisApp::Image> VKRenderContext::CaptureRenderTarget(RenderTarget* surface) const
{
    struct VKTexture: public Texture
    {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;

        uint32_t width;
        uint32_t height;
    };

    VKTexture* texture = (VKTexture*)surface->GetTexture();

    // Buffer creation
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = texture->width * texture->height * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    V(vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    VkDeviceMemory memory;
    V(vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory));
    V(vkBindBufferMemory(mDevice, buffer, memory, 0));

    // Map buffer memory
    void* data;
    V(vkMapMemory(mDevice, memory, 0, VK_WHOLE_SIZE, 0, &data));

    // Layout transition
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = texture->image;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(mCommandBuffers[mFrameIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy image
    VkBufferImageCopy region{};
    region.imageExtent.width = texture->width;
    region.imageExtent.height = texture->height;
    region.imageExtent.depth = 1;
    region.imageSubresource.mipLevel = 0;
    region.bufferOffset = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCmdCopyImageToBuffer(mCommandBuffers[mFrameIndex], texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

  #ifdef NS_PROFILE
    if (mHasTimeStampQueries)
    {
        VkCommandBuffer commandBuffer = mCommandBuffers[mFrameIndex];
        VkQueryPool queryPool = mTimeStampQueries[mFrameIndex];

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
    }
  #endif

    VK_END_EVENT();

    // Submit commands
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[mFrameIndex];

    V(vkEndCommandBuffer(mCommandBuffers[mFrameIndex]));
    V(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    vkDeviceWaitIdle(mDevice);

    Ptr<Image> image = *new Image(texture->width, texture->height);

    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dst = (uint8_t*)image->Data();

    for (uint32_t i = 0; i < texture->height; i++)
    {
        for (uint32_t j = 0; j < texture->width; j++)
        {
            // RGBA -> BGRA
            dst[4 * j + 0] = src[4 * j + 2];
            dst[4 * j + 1] = src[4 * j + 1];
            dst[4 * j + 2] = src[4 * j + 0];
            dst[4 * j + 3] = src[4 * j + 3];
        }

        src += texture->width * 4;
        dst += texture->width * 4;
    }

    vkDestroyBuffer(mDevice, buffer, nullptr);
    vkFreeMemory(mDevice, memory, nullptr);

    return image;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::Swap()
{
    vkCmdEndRenderPass(mCommandBuffers[mFrameIndex]);

  #ifdef NS_PROFILE
    if (mHasTimeStampQueries)
    {
        VkCommandBuffer commandBuffer = mCommandBuffers[mFrameIndex];
        VkQueryPool queryPool = mTimeStampQueries[mFrameIndex];

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 1);
    }
  #endif

    VK_END_EVENT();
    VK_END_EVENT();

    V(vkEndCommandBuffer(mCommandBuffers[mFrameIndex]));

    if (mImageAcquired)
    {
        // The optimal value is usually VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, because only
        // need the swapchain image to be ready when we are going to write to it
        VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &mImageAvailableSemaphores[mFrameIndex];
        submitInfo.pWaitDstStageMask = &waitDstStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffers[mFrameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[mFrameIndex];

        V(vkResetFences(mDevice, 1, &mInFlightFences[mFrameIndex]));
        V(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mFrameIndex]));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mFrameIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &mSwapChain;
        presentInfo.pImageIndices = &mImageIndex;

        VkResult r = vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
        NS_ASSERT(r == VK_SUCCESS || r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR ||
            r == VK_ERROR_SURFACE_LOST_KHR);

        mFrameIndex = (mFrameIndex + 1) % FramesInFlight;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* VKRenderContext::CreatePixelShader(const char* label, uint8_t shader, ArrayRef<uint8_t> code)
{
    return VKFactory::CreatePixelShader(mRenderer, label, (uint8_t)shader, code.Data(), code.Size());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool FindExtension(BaseVector<VkExtensionProperties>& v, const char* name)
{
    for (VkExtensionProperties& extension : v)
    {
        if (StrEquals(extension.extensionName, name))
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t VKRenderContext::FindMemoryType(uint32_t typeBits, uint32_t required) const
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
uint32_t VKRenderContext::FindMemoryType(uint32_t typeBits, uint32_t optimal, uint32_t required) const
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
void VKRenderContext::CreateTransientImage(const char* label, uint32_t width, uint32_t height,
    VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImage& image,
    VkDeviceMemory& memory, VkImageView& view) const
{
    // Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    imageInfo.samples = mSampleCount;

    V(vkCreateImage(mDevice, &imageInfo, nullptr, &image));
    VK_NAME(image, IMAGE, "%s", label);

    // Memory
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mDevice, image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    V(vkAllocateMemory(mDevice, &allocInfo, nullptr, &memory));
    V(vkBindImageMemory(mDevice, image, memory, 0));
    VK_NAME(memory, DEVICE_MEMORY, "%s_Memory", label);
    NS_LOG_TRACE("Image '%s' created (%llu KB) (Type %02d)", label, allocInfo.allocationSize / 1024,
        allocInfo.memoryTypeIndex);

    // View
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = image;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    V(vkCreateImageView(mDevice, &viewInfo, nullptr, &view));
    VK_NAME(view, IMAGE_VIEW, "%s_View", label);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VkFormat VKRenderContext::FindStencilFormat() const
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
        NS_LOG_TRACE(" Stencil Format: S8_UINT");
        return VK_FORMAT_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D32_SFLOAT_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        NS_LOG_TRACE(" Stencil Format: D32_SFLOAT_S8_UINT");
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D24_UNORM_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        NS_LOG_TRACE(" Stencil Format: D24_UNORM_S8_UINT");
        return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, VK_FORMAT_D16_UNORM_S8_UINT, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        NS_LOG_TRACE(" Stencil Format: D16_UNORM_S8_UINT");
        return VK_FORMAT_D16_UNORM_S8_UINT;
    }

    NS_ASSERT_UNREACHABLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VkFormat VKRenderContext::FindBackBufferFormat(bool sRGB) const
{
    NS_ASSERT(mPhysicalDevice != VK_NULL_HANDLE);
    NS_ASSERT(mSurface != VK_NULL_HANDLE);

    uint32_t count;
    V(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &count, nullptr));

    Vector<VkSurfaceFormatKHR, 32> formats(count);
    V(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &count, formats.Data()));

    // https://vulkan.gpuinfo.org/listsurfaceformats.php
    // B8G8R8A8 or R8G8B8A8 with SRGB_NONLINEAR should be supported everywhere

    for (VkSurfaceFormatKHR x : formats)
    {
        if (x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            if (sRGB)
            {
                if (x.format == VK_FORMAT_B8G8R8A8_SRGB)
                {
                    NS_LOG_TRACE(" BackBuffer Format: B8G8R8A8_SRGB");
                    return x.format;
                }

                if (x.format == VK_FORMAT_R8G8B8A8_SRGB)
                {
                    NS_LOG_TRACE(" BackBuffer Format: R8G8B8A8_SRGB");
                    return x.format;
                }
            }
            else
            {
                if (x.format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                    NS_LOG_TRACE(" BackBuffer Format: B8G8R8A8_UNORM");
                    return x.format;
                }

                if (x.format == VK_FORMAT_R8G8B8A8_UNORM)
                {
                    NS_LOG_TRACE(" BackBuffer Format: R8G8B8A8_UNORM");
                    return x.format;
                }
            }
        }
    }

    NS_ASSERT_UNREACHABLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VkPresentModeKHR VKRenderContext::FindPresentMode(bool vSync) const
{
    NS_ASSERT(mPhysicalDevice != VK_NULL_HANDLE);
    NS_ASSERT(mSurface != VK_NULL_HANDLE);

    uint32_t count;
    V(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &count, nullptr));

    Vector<VkPresentModeKHR, 8> modes(count);
    V(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &count, modes.Data()));

    for (VkPresentModeKHR x : modes)
    {
        if (!vSync && x == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            NS_LOG_TRACE(" Present Mode: IMMEDIATE_KHR");
            return x;
        }
    }

    // FIFO is is required to be supported
    NS_LOG_TRACE(" Present Mode: FIFO_KHR");
    return VK_PRESENT_MODE_FIFO_KHR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::GetInstanceExtensions(BaseVector<const char*>& extensionNames)
{
    uint32_t extensionCount = 0;
    V(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

    Vector<VkExtensionProperties, 64> extensions(extensionCount);
    V(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.Data()));

    NS_ASSERT(FindExtension(extensions, VK_KHR_SURFACE_EXTENSION_NAME));
    extensionNames.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    NS_ASSERT(FindExtension(extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME));
    extensionNames.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
  #endif

  #ifdef NS_PLATFORM_ANDROID
    NS_ASSERT(FindExtension(extensions, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME));
    extensionNames.PushBack(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
  #endif

  #ifdef NS_PROFILE
    if (FindExtension(extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
    {
        extensionNames.PushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::GetDeviceExtensions(BaseVector<const char*>& extensionNames)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extensionCount, nullptr);

    Vector<VkExtensionProperties, 256> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extensionCount, extensions.Data());

    NS_ASSERT(FindExtension(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    extensionNames.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  #ifdef NS_PROFILE
    if (FindExtension(extensions, VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
    {
        extensionNames.PushBack(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateInstance()
{
    Vector<const char*, 16> extensions;
    GetInstanceExtensions(extensions);

    NS_LOG_TRACE(" = Instance extensions =");
    for (const char* name : extensions) { NS_LOG_TRACE("  %s", name); }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.Size();
    createInfo.ppEnabledExtensionNames = extensions.Data();

    V(vkCreateInstance(&createInfo, nullptr, &mInstance));

    LOAD_INSTANCE_FUNC(vkDestroyInstance);
    LOAD_INSTANCE_FUNC(vkGetDeviceProcAddr);
    LOAD_INSTANCE_FUNC(vkCreateDevice);
    LOAD_INSTANCE_FUNC(vkDestroyDevice);
    LOAD_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
    LOAD_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties);

    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    LOAD_INSTANCE_FUNC(vkDestroySurfaceKHR);

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    LOAD_INSTANCE_FUNC(vkCreateWin32SurfaceKHR);
  #endif

  #ifdef NS_PLATFORM_ANDROID
    LOAD_INSTANCE_FUNC(vkCreateAndroidSurfaceKHR);
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyInstance()
{
    vkDestroyInstance(mInstance, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateSurface(void* window)
{
    mWindow = window;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)window;
    createInfo.hinstance = GetModuleHandle(nullptr);

    V(vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface));
  #endif

  #ifdef NS_PLATFORM_ANDROID
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = (ANativeWindow*)window;

    V(vkCreateAndroidSurfaceKHR(mInstance, &createInfo, nullptr, &mSurface));
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroySurface()
{
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    mSurface = VK_NULL_HANDLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static VkSampleCountFlagBits GetSampleCount(uint32_t& samples, const VkPhysicalDeviceLimits& limits)
{
    auto sampleCounts = limits.framebufferColorSampleCounts & limits.framebufferStencilSampleCounts;

    for (uint32_t bits = VK_SAMPLE_COUNT_64_BIT; bits > VK_SAMPLE_COUNT_1_BIT; bits >>= 1)
    {
        if (samples >= bits && (sampleCounts & bits) > 0)
        {
            samples = bits;
            return (VkSampleCountFlagBits)bits;
        }
    }

    samples = 1;
    return VK_SAMPLE_COUNT_1_BIT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateDevice(uint32_t& samples, bool sRGB, bool vSync)
{
    // Physical device (get first one for now)
    uint32_t deviceCount = 1;
    V(vkEnumeratePhysicalDevices(mInstance, &deviceCount, &mPhysicalDevice));
    NS_ASSERT(deviceCount >= 1);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);
    NS_LOG_DEBUG(" Physical Device: %s", deviceProperties.deviceName);
    NS_LOG_DEBUG(" API version: Vulkan %d.%d", VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
        VK_API_VERSION_MINOR(deviceProperties.apiVersion));

    mSampleCount = GetSampleCount(samples, deviceProperties.limits);
    mBackBufferFormat = FindBackBufferFormat(sRGB);
    mStencilFormat = FindStencilFormat();
    mPresentMode = FindPresentMode(vSync);

    if (deviceProperties.limits.timestampComputeAndGraphics)
    {
        mHasTimeStampQueries = true;
        mTimeStampPeriod = deviceProperties.limits.timestampPeriod;
    }

    // Memory properties
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);

    NS_LOG_TRACE(" = Memory Heaps =");
    for (uint32_t i = 0; i < mMemoryProperties.memoryHeapCount; i++)
    {
        auto& memoryHeap = mMemoryProperties.memoryHeaps[i];

        NS_LOG_TRACE("  Heap %d - %" PRIu64 " MB (%s%s )", i, memoryHeap.size / (1024 * 1024),
            memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? " DEVICE_LOCAL" : "",
            memoryHeap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT ? " MULTI_INSTANCE" : ""
        );

        for (uint32_t j = 0; j < mMemoryProperties.memoryTypeCount; j++)
        {
            auto& memoryType = mMemoryProperties.memoryTypes[j];

            if (memoryType.heapIndex == i && memoryType.propertyFlags)
            {
                NS_LOG_TRACE("   + Type %02d:%s%s%s%s%s", j,
                    memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? " DEVICE_LOCAL" : "",
                    memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? " HOST_VISIBLE" : "",
                    memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? " HOST_COHERENT" : "",
                    memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? " HOST_CACHED" : "",
                    memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? " LAZILY_ALLOCATED" : ""
                );
            }
        }
    }

    // Queue families (select first one with support for graphics and present)
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

    Vector<VkQueueFamilyProperties, 32> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies.Data());

    mQueueFamilyIndex = 0xFFFFFFFF;

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 presentSupport = false;
        V(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface, &presentSupport));

        if (presentSupport && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            mQueueFamilyIndex = i;
            break;
        }
    }

    NS_ASSERT(mQueueFamilyIndex != 0xFFFFFFFF);

    // Logical device
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = mQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    Vector<const char*, 16> extensions;
    GetDeviceExtensions(extensions);

    NS_LOG_TRACE(" = Device extensions =");
    for (const char* name : extensions) { NS_LOG_TRACE("  %s", name); }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = extensions.Data();
    createInfo.enabledExtensionCount = extensions.Size();

    V(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice));

    LOAD_DEVICE_FUNC(vkGetDeviceQueue);
    LOAD_DEVICE_FUNC(vkAllocateMemory);
    LOAD_DEVICE_FUNC(vkFreeMemory);
    LOAD_DEVICE_FUNC(vkCreateImage);
    LOAD_DEVICE_FUNC(vkDestroyImage);
    LOAD_DEVICE_FUNC(vkGetImageMemoryRequirements);
    LOAD_DEVICE_FUNC(vkBindImageMemory);
    LOAD_DEVICE_FUNC(vkCreateImageView);
    LOAD_DEVICE_FUNC(vkDestroyImageView);
    LOAD_DEVICE_FUNC(vkCreateRenderPass);
    LOAD_DEVICE_FUNC(vkDestroyRenderPass);
    LOAD_DEVICE_FUNC(vkCreateFramebuffer);
    LOAD_DEVICE_FUNC(vkDestroyFramebuffer);
    LOAD_DEVICE_FUNC(vkCreateCommandPool);
    LOAD_DEVICE_FUNC(vkDestroyCommandPool);
    LOAD_DEVICE_FUNC(vkAllocateCommandBuffers);
    LOAD_DEVICE_FUNC(vkResetCommandPool);
    LOAD_DEVICE_FUNC(vkBeginCommandBuffer);
    LOAD_DEVICE_FUNC(vkEndCommandBuffer);
    LOAD_DEVICE_FUNC(vkCmdBeginRenderPass);
    LOAD_DEVICE_FUNC(vkCmdEndRenderPass);
    LOAD_DEVICE_FUNC(vkCreateSemaphore);
    LOAD_DEVICE_FUNC(vkDestroySemaphore);
    LOAD_DEVICE_FUNC(vkCreateFence);
    LOAD_DEVICE_FUNC(vkDestroyFence);
    LOAD_DEVICE_FUNC(vkWaitForFences);
    LOAD_DEVICE_FUNC(vkResetFences);
    LOAD_DEVICE_FUNC(vkQueueSubmit);
    LOAD_DEVICE_FUNC(vkDeviceWaitIdle);
    LOAD_DEVICE_FUNC(vkCreateQueryPool);
    LOAD_DEVICE_FUNC(vkDestroyQueryPool);
    LOAD_DEVICE_FUNC(vkCmdResetQueryPool);
    LOAD_DEVICE_FUNC(vkCmdWriteTimestamp);
    LOAD_DEVICE_FUNC(vkGetQueryPoolResults);
    LOAD_DEVICE_FUNC(vkCreateSwapchainKHR);
    LOAD_DEVICE_FUNC(vkDestroySwapchainKHR);
    LOAD_DEVICE_FUNC(vkGetSwapchainImagesKHR);
    LOAD_DEVICE_FUNC(vkAcquireNextImageKHR);
    LOAD_DEVICE_FUNC(vkQueuePresentKHR);
    LOAD_DEVICE_FUNC(vkCmdSetViewport);
    LOAD_DEVICE_FUNC(vkCmdSetScissor);
    LOAD_DEVICE_FUNC(vkCmdCopyImageToBuffer);
    LOAD_DEVICE_FUNC(vkBindBufferMemory);
    LOAD_DEVICE_FUNC(vkCreateBuffer);
    LOAD_DEVICE_FUNC(vkDestroyBuffer);
    LOAD_DEVICE_FUNC(vkGetBufferMemoryRequirements);
    LOAD_DEVICE_FUNC(vkCmdPipelineBarrier);
    LOAD_DEVICE_FUNC(vkMapMemory);
    LOAD_DEVICE_FUNC(vkCreatePipelineCache);
    LOAD_DEVICE_FUNC(vkGetPipelineCacheData);
    LOAD_DEVICE_FUNC(vkDestroyPipelineCache);

    LOAD_DEVICE_EXT_FUNC(vkCmdDebugMarkerBeginEXT);
    LOAD_DEVICE_EXT_FUNC(vkCmdDebugMarkerEndEXT);
    LOAD_DEVICE_EXT_FUNC(vkDebugMarkerSetObjectNameEXT);

    vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mGraphicsQueue);

    //VK_NAME(mInstance, INSTANCE, "Noesis_Instance"); // [RenderDoc]
    //VK_NAME(mPhysicalDevice, PHYSICAL_DEVICE, "Noesis_Physical_Device"); // [RenderDoc]
    VK_NAME(mDevice, DEVICE, "Noesis_Device");
    VK_NAME(mGraphicsQueue, QUEUE, "Noesis_Queue");
    //VK_NAME(mSurface, SURFACE_KHR, "Noesis_Surface"); // [PowerVR GE8320]
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyDevice()
{
    vkDestroyDevice(mDevice, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateRenderPass()
{
    VkAttachmentReference stencilRef{ 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    VkAttachmentReference colorRef{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference resolveRef{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &stencilRef;

    Vector<VkAttachmentDescription, 3> attachments;

    // Stencil: Clear -> Don't care
    VkAttachmentDescription stencil{};
    stencil.samples = mSampleCount;
    stencil.format = mStencilFormat;
    stencil.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    stencil.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencil.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencil.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencil.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    stencil.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments.PushBack(stencil);

    if (mSampleCount > 1)
    {
        // Multisampled Color: Clear -> Don't care
        VkAttachmentDescription colorAA{};
        colorAA.format = mBackBufferFormat;
        colorAA.samples = mSampleCount;
        colorAA.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAA.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAA.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAA.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAA.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAA.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.PushBack(colorAA);

        // Resolved Color: Don't care -> Store
        VkAttachmentDescription color{};
        color.format = mBackBufferFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.PushBack(color);

        subpass.pResolveAttachments = &resolveRef;
    }
    else
    {
        // Color: Clear -> Store
        VkAttachmentDescription color{};
        color.format = mBackBufferFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.PushBack(color);
    }

    // The acquisition semaphore's pWaitDstStageMask guarantees that the image acquisition happens
    // before VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, but we do not know exactly when. Thus
    // we need an external dependency which fixes the stage for the transition to
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT. Otherwise, the GPU might try to transition
    // the image before it is acquired from the presentation engine.

    // For the stencil, we also need VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT to wait for
    // the clear operation before starting to write and read it (Write-After-Write hazard)

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.Size();
    renderPassInfo.pAttachments = attachments.Data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    V(vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass));
    VK_NAME(mRenderPass, RENDER_PASS, "Noesis_FB_RenderPass");

    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    V(vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPassNoClear));
    VK_NAME(mRenderPass, RENDER_PASS, "Noesis_FB_RenderPass_NoClear");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyRenderPass()
{
    vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
    vkDestroyRenderPass(mDevice, mRenderPassNoClear, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateSwapChain()
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    V(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &surfaceCaps));

    uint32_t maxImageCount = !surfaceCaps.maxImageCount ? UINT32_MAX : surfaceCaps.maxImageCount;

    // Note that currentExtent can have the special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that
    // the surface size will be determined by the extent of a swapchain targeting the surface
    NS_ASSERT(surfaceCaps.currentExtent.width != 0xFFFFFFFF);
    mSwapChainExtent = surfaceCaps.currentExtent;

    VkSwapchainKHR oldSwapchain = mSwapChain;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = Min(surfaceCaps.minImageCount + 1, maxImageCount);
    createInfo.imageFormat = mBackBufferFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = mSwapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = surfaceCaps.currentTransform;
    createInfo.presentMode = mPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = mSwapChain;

    if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    VkCompositeAlphaFlagBitsKHR alphaFlags[] =
    {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
    };

    for (VkCompositeAlphaFlagBitsKHR alphaFlag : alphaFlags)
    {
        if (surfaceCaps.supportedCompositeAlpha & alphaFlag)
        {
            createInfo.compositeAlpha = alphaFlag;
            break;
        }
    }

    NS_ASSERT(createInfo.compositeAlpha != 0);

    V(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain));
    //VK_NAME(mSwapChain, SWAPCHAIN_KHR, "Noesis_FB_SwapChain"); // [PowerVR GE8320]

    // If we just re-created an existing swapchain, destroy the old swapchain at this point
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(mDevice, oldSwapchain, nullptr);
    }

    V(vkGetSwapchainImagesKHR(mDevice, mSwapChain, &mSwapChainImagesCount, nullptr));
    mSwapChainImagesCount = Min<uint32_t>(mSwapChainImagesCount, NS_COUNTOF(mSwapChainImages));
    V(vkGetSwapchainImagesKHR(mDevice, mSwapChain, &mSwapChainImagesCount, mSwapChainImages));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroySwapChain()
{
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
    mSwapChain = VK_NULL_HANDLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateImageViews()
{
    NS_ASSERT(mSwapChain != VK_NULL_HANDLE);

    CreateTransientImage("Noesis_FB_Stencil", mSwapChainExtent.width, mSwapChainExtent.height,
        mStencilFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_STENCIL_BIT,
        mStencilImage, mStencilMemory, mStencilView);

    if (mSampleCount > 1)
    {
        CreateTransientImage("Noesis_FB_ColorAA", mSwapChainExtent.width, mSwapChainExtent.height,
            mBackBufferFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            mColorAAImage, mColorAAMemory, mColorAAView);
    }

    for (uint32_t i = 0; i < mSwapChainImagesCount; i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.image = mSwapChainImages[i];
        viewInfo.format = mBackBufferFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        V(vkCreateImageView(mDevice, &viewInfo, nullptr, &mSwapChainImageViews[i]));
        VK_NAME(mSwapChainImages[i], IMAGE, "Noesis_FB_Color_%d", i);
        VK_NAME(mSwapChainImageViews[i], IMAGE_VIEW, "Noesis_FB_Color_View_%d", i);

        Vector<VkImageView, 3> attachmentViews;
        attachmentViews.PushBack(mStencilView);
        if (mSampleCount > 1) { attachmentViews.PushBack(mColorAAView); }
        attachmentViews.PushBack(mSwapChainImageViews[i]);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = attachmentViews.Size();
        framebufferInfo.pAttachments = attachmentViews.Data();
        framebufferInfo.width = mSwapChainExtent.width;
        framebufferInfo.height = mSwapChainExtent.height;
        framebufferInfo.layers = 1;

        V(vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFrameBuffers[i]));
        VK_NAME(mSwapChainFrameBuffers[i], FRAMEBUFFER, "Noesis_FB_%d", i);
    }

    mViewport.x = 0.0f;
    mViewport.y = 0.0f;
    mViewport.width = (float)Max(1U, mSwapChainExtent.width);
    mViewport.height = (float)Max(1U, mSwapChainExtent.height);
    mViewport.minDepth = 0.0f;
    mViewport.maxDepth = 1.0f;

    mScissorRect.offset.x = 0;
    mScissorRect.offset.y = 0;
    mScissorRect.extent.width = mSwapChainExtent.width;
    mScissorRect.extent.height = mSwapChainExtent.height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyImageViews()
{
    for (uint32_t i = 0; i < mSwapChainImagesCount; i++)
    {
        vkDestroyImageView(mDevice, mSwapChainImageViews[i], nullptr);
        mSwapChainImageViews[i] = VK_NULL_HANDLE;
        vkDestroyFramebuffer(mDevice, mSwapChainFrameBuffers[i], nullptr);
        mSwapChainFrameBuffers[i] = VK_NULL_HANDLE;
    }

    vkDestroyImageView(mDevice, mStencilView, nullptr);
    mStencilView = VK_NULL_HANDLE;
    vkDestroyImage(mDevice, mStencilImage, nullptr);
    mStencilImage = VK_NULL_HANDLE;
    vkFreeMemory(mDevice, mStencilMemory, nullptr);
    mStencilMemory = VK_NULL_HANDLE;

    vkDestroyImageView(mDevice, mColorAAView, nullptr);
    mColorAAView = VK_NULL_HANDLE;
    vkDestroyImage(mDevice, mColorAAImage, nullptr);
    mColorAAImage = VK_NULL_HANDLE;
    vkFreeMemory(mDevice, mColorAAMemory, nullptr);
    mColorAAMemory = VK_NULL_HANDLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateCommands()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = mQueueFamilyIndex;

    for (uint32_t i = 0; i < FramesInFlight; i++)
    {
        V(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPools[i]));
        VK_NAME(mCommandPools[i], COMMAND_POOL, "Noesis_Commands_Pool_%d", i);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = mCommandPools[i];
        allocInfo.commandBufferCount = 1;

        V(vkAllocateCommandBuffers(mDevice, &allocInfo, &mCommandBuffers[i]));
        VK_NAME(mCommandBuffers[i], COMMAND_BUFFER, "Noesis_Commands_Buffer_%d", i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyCommands()
{
    for (uint32_t i = 0; i < FramesInFlight; i++)
    {
        vkDestroyCommandPool(mDevice, mCommandPools[i], nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < FramesInFlight; i++)
    {
        V(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]));
        V(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]));
        V(vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]));

        VK_NAME(mImageAvailableSemaphores[i], SEMAPHORE, "Noesis_Semaphore_Start_%d", i);
        VK_NAME(mRenderFinishedSemaphores[i], SEMAPHORE, "Noesis_Semaphore_End_%d", i);
        VK_NAME(mInFlightFences[i], FENCE, "Noesis_Fence_%d", i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroySyncObjects()
{
    for (uint32_t i = 0; i < FramesInFlight; i++)
    {
        vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreateQueries()
{
    if (mHasTimeStampQueries)
    {
        VkQueryPoolCreateInfo queryInfo{};
        queryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryInfo.queryCount = 2;

        for (uint32_t i = 0; i < FramesInFlight; i++)
        {
            V(vkCreateQueryPool(mDevice, &queryInfo, nullptr, &mTimeStampQueries[i]));
            VK_NAME(mTimeStampQueries[i], QUERY_POOL, "Noesis_Query_Pool_%d", i);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static FILE* GetPipelineCacheFile(bool write, const VkPhysicalDeviceProperties& props)
{
  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    WCHAR shaderCachePath[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, shaderCachePath);
    NS_ASSERT(!FAILED(hr));

    wcscat_s(shaderCachePath, L"\\Noesis Technologies");
    CreateDirectoryW(shaderCachePath, nullptr);

    wcscat_s(shaderCachePath, L"\\Vulkan");
    CreateDirectoryW(shaderCachePath, nullptr);

    WCHAR vendor[128];
    _snwprintf_s(vendor, 128, L"\\PipelineCache.%x.%x", props.vendorID, props.deviceID);
    wcscat_s(shaderCachePath, vendor);

    FILE* fp = nullptr;
    _wfopen_s(&fp, shaderCachePath, write ? L"wb" : L"rb");
    return fp;
  #endif

  #ifdef NS_PLATFORM_ANDROID
    android_app *app = (android_app*)Display::GetPrivateData();

    JNIEnv* env;
    app->activity->vm->AttachCurrentThread(&env, NULL);

    jclass activityClass = env->FindClass("android/app/NativeActivity");
    jmethodID getCacheDir = env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
    jobject dir = env->CallObjectMethod(app->activity->clazz, getCacheDir);

    jclass fileClass = env->FindClass("java/io/File");
    jmethodID getPath = env->GetMethodID(fileClass, "getPath", "()Ljava/lang/String;");
    jstring str = (jstring)env->CallObjectMethod(dir, getPath);

    const char* chars = env->GetStringUTFChars(str, NULL);
    FixedString<256> cachePath(chars);
    env->ReleaseStringUTFChars(str, chars);
    app->activity->vm->DetachCurrentThread();

    cachePath.AppendFormat("/noesis_pipeline_cache.%x.%x", props.vendorID, props.deviceID);

    return fopen(cachePath.Str(), write ? "wb" : "rb");
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsPipelineCacheValid(void* ptr, uint32_t size, const VkPhysicalDeviceProperties& props)
{
    if (size >= 32)
    {
        uint32_t* data = (uint32_t*)ptr;
        uint32_t headerSize = *data++;

        if (headerSize == 32)
        {
            uint32_t headerVersion = *data++;
            if (headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
            {
                uint32_t vendorID = *data++;
                if (vendorID == props.vendorID)
                {
                    uint32_t deviceID = *data++;
                    if (deviceID == props.deviceID)
                    {
                        uint8_t* pipelineCacheUUID = (uint8_t*)data;
                        if (memcmp(props.pipelineCacheUUID, pipelineCacheUUID, VK_UUID_SIZE) == 0)
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::CreatePipelineCache()
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);

    FILE* fp = GetPipelineCacheFile(false, deviceProperties);
    if (fp != 0)
    {
        uint64_t t0 = HighResTimer::Ticks();

        fseek(fp, 0, SEEK_END);
        uint32_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        void* data = Alloc(size);
        fread(data, size, 1, fp);
        fclose(fp);

        if (IsPipelineCacheValid(data, size, deviceProperties))
        {
            VkPipelineCacheCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            createInfo.initialDataSize = size;
            createInfo.pInitialData = data;

            V(vkCreatePipelineCache(mDevice, &createInfo, nullptr, &mPipelineCache));

            uint64_t t1 = HighResTimer::Ticks();
            NS_LOG_TRACE("Pipeline cache loaded in %.0f ms", HighResTimer::Milliseconds(t1 - t0));
        }
        else
        {
            NS_LOG_WARNING("Invalid Pipeline cache found");
        }

        Dealloc(data);
    }
    else
    {
        NS_LOG_WARNING("Pipeline cache not found");
    }

    if (mPipelineCache == VK_NULL_HANDLE)
    {
        VkPipelineCacheCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        V(vkCreatePipelineCache(mDevice, &createInfo, nullptr, &mPipelineCache));
    }

    VK_NAME(mPipelineCache, PIPELINE_CACHE, "Noesis_Pipeline_Cache");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::SavePipelineCache()
{
    size_t size = 0;
    V(vkGetPipelineCacheData(mDevice, mPipelineCache, &size, nullptr));

    if (size >= 32)
    {
        void* data = Alloc(size);
        VkResult hr = vkGetPipelineCacheData(mDevice, mPipelineCache, &size, data);

        if (hr == VK_SUCCESS)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);

            FILE* fp = GetPipelineCacheFile(true, deviceProperties);

            if (fp != 0)
            {
                fwrite(data, size, 1, fp);
                fclose(fp);

                NS_LOG_TRACE("Saved pipeline cache data (%zu bytes)", size);
            }
            else
            {
                NS_LOG_WARNING("Failed to save pipeline cache data (%zu bytes)", size);
            }
        }
        else
        {
            NS_LOG_WARNING("Failed to get pipeline cache data (HR=%d, %zu bytes)", hr, size);
        }

        Dealloc(data);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyPipelineCache()
{
    SavePipelineCache();
    vkDestroyPipelineCache(mDevice, mPipelineCache, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VKRenderContext::DestroyQueries()
{
    if (mHasTimeStampQueries)
    {
        for (uint32_t i = 0; i < FramesInFlight; i++)
        {
            vkDestroyQueryPool(mDevice, mTimeStampQueries[i], nullptr);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(VKRenderContext)
{
    NsMeta<Category>("RenderContext");
}

NS_END_COLD_REGION
