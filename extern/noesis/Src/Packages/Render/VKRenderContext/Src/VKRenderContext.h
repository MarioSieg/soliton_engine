////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_VKRENDERCONTEXT_H__
#define __RENDER_VKRENDERCONTEXT_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsRender/RenderContext.h>

#include <NsRender/vulkan/vulkan_core.h>

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <NsRender/vulkan/vulkan_win32.h>
#endif

#ifdef NS_PLATFORM_ANDROID
#include <NsRender/vulkan/vulkan_android.h>
#endif

namespace Noesis { class RenderDevice; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class VKRenderContext final: public RenderContext
{
public:
    VKRenderContext();
    ~VKRenderContext();

    /// From RenderContext
    //@{
    const char* Description() const override;
    const char* ShaderLang() const override;
    uint32_t Score() const override;
    bool Validate() const override;
    void Init(void* window, uint32_t& samples, bool vSync, bool sRGB) override;
    void Shutdown() override;
    void SetWindow(void* window) override;
    void SaveState() override;
    Noesis::RenderDevice* GetDevice() const override;
    void BeginRender() override;
    void EndRender() override;
    void Resize() override;
    float GetGPUTimeMs() const override;
    void SetClearColor(float r, float g, float b, float a) override;
    void SetDefaultRenderTarget(uint32_t width, uint32_t height, bool doClearColor) override;
    Noesis::Ptr<Image> CaptureRenderTarget(Noesis::RenderTarget* surface) const override;
    void Swap() override;
    void* CreatePixelShader(const char* label, uint8_t shader,
        Noesis::ArrayRef<uint8_t> code) override;
    //@}

private:
    uint32_t FindMemoryType(uint32_t typeBits, uint32_t required) const;
    uint32_t FindMemoryType(uint32_t typeBits, uint32_t optimal, uint32_t required) const;

    void CreateTransientImage(const char* label, uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImage& image, VkDeviceMemory& memory,
        VkImageView& view) const;

    VkFormat FindStencilFormat() const;
    VkFormat FindBackBufferFormat(bool sRGB) const;
    VkPresentModeKHR FindPresentMode(bool vSync) const;

    void GetInstanceExtensions(Noesis::BaseVector<const char*>& extensionNames);
    void GetDeviceExtensions(Noesis::BaseVector<const char*>& extensionNames);

    void CreateInstance();
    void DestroyInstance();

    void CreateSurface(void* window);
    void DestroySurface();

    void CreateDevice(uint32_t& samples, bool sRGB, bool vSync);
    void DestroyDevice();

    void CreateRenderPass();
    void DestroyRenderPass();

    void CreateSwapChain();
    void DestroySwapChain();

    void CreateImageViews();
    void DestroyImageViews();

    void CreateCommands();
    void DestroyCommands();

    void CreateSyncObjects();
    void DestroySyncObjects();

    void CreateQueries();
    void DestroyQueries();

    void CreatePipelineCache();
    void SavePipelineCache();
    void DestroyPipelineCache();

private:
    void* mVulkanLib = 0;

    // Global funcs
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;

    // Instance funcs
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
  #endif

  #ifdef NS_PLATFORM_ANDROID
    PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
  #endif

    // Device funcs
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkDestroyImage vkDestroyImage;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkDestroyImageView vkDestroyImageView;
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkDestroyRenderPass vkDestroyRenderPass;
    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkDestroyCommandPool vkDestroyCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkResetCommandPool vkResetCommandPool;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkDestroySemaphore vkDestroySemaphore;
    PFN_vkCreateFence vkCreateFence;
    PFN_vkDestroyFence vkDestroyFence;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkResetFences vkResetFences;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
    PFN_vkCreateQueryPool vkCreateQueryPool;
    PFN_vkDestroyQueryPool vkDestroyQueryPool;
    PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
    PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
    PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkCreatePipelineCache vkCreatePipelineCache;
    PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
    PFN_vkDestroyPipelineCache vkDestroyPipelineCache;

    PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
    PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;

    Noesis::Ptr<Noesis::RenderDevice> mRenderer;

    VkInstance mInstance = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkRenderPass mRenderPassNoClear = VK_NULL_HANDLE;
    VkPipelineCache mPipelineCache = VK_NULL_HANDLE;

    VkPhysicalDeviceMemoryProperties mMemoryProperties;

    uint32_t mQueueFamilyIndex;

    VkClearValue mClearValues[2] = {};
    VkFormat mBackBufferFormat;
    VkFormat mStencilFormat;
    VkSampleCountFlagBits mSampleCount;
    VkPresentModeKHR mPresentMode;
    uint32_t mImageIndex;
    bool mImageAcquired;
    void* mWindow;

    VkViewport mViewport;
    VkRect2D mScissorRect;

    VkImage mStencilImage = VK_NULL_HANDLE;
    VkDeviceMemory mStencilMemory = VK_NULL_HANDLE;
    VkImageView mStencilView = VK_NULL_HANDLE;

    VkImage mColorAAImage = VK_NULL_HANDLE;
    VkDeviceMemory mColorAAMemory = VK_NULL_HANDLE;
    VkImageView mColorAAView = VK_NULL_HANDLE;

    VkImage mSwapChainImages[8] = {};
    VkImageView mSwapChainImageViews[8] = {};
    VkFramebuffer mSwapChainFrameBuffers[8] = {};
    uint32_t mSwapChainImagesCount = 0;
    VkExtent2D mSwapChainExtent;

    // The maximum number of frames that will be queued to the GPU at a time
    static constexpr uint32_t FramesInFlight = 2;

    VkCommandPool mCommandPools[FramesInFlight];
    VkCommandBuffer mCommandBuffers[FramesInFlight];
    VkSemaphore mImageAvailableSemaphores[FramesInFlight];
    VkSemaphore mRenderFinishedSemaphores[FramesInFlight];
    VkFence mInFlightFences[FramesInFlight];
    VkQueryPool mTimeStampQueries[FramesInFlight];

    bool mHasTimeStampQueries = false;
    float mTimeStampPeriod = 0.0f;
    float mGPUTimeMs = 0.0f;

    uint32_t mFrameIndex = 0;
    uint64_t mFrameNumber = 0;

    NS_DECLARE_REFLECTION(VKRenderContext, RenderContext)
};

}

#endif
