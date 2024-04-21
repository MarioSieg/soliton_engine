////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_VKRENDERDEVICE_H__
#define __RENDER_VKRENDERDEVICE_H__


#include <NsRender/RenderDevice.h>
#include <NsRender/VKFactory.h>
#include <NsCore/HashMap.h>
#include <NsCore/Vector.h>
#include <NsCore/String.h>


namespace Noesis { template<class T> class Ptr; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// VKRenderDevice
////////////////////////////////////////////////////////////////////////////////////////////////////
class VKRenderDevice final: public Noesis::RenderDevice
{
public:
    VKRenderDevice(bool sRGB, const VKFactory::InstanceInfo& info);
    ~VKRenderDevice();

    void SetCommandBuffer(const VKFactory::RecordingInfo& info);
    void SetRenderPass(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount);
    void WarmUpRenderPass(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount);

    static Noesis::Ptr<Noesis::Texture> WrapTexture(VkImage image, uint32_t width, uint32_t height,
        uint32_t levels, VkFormat format, VkImageLayout layout, bool isInverted, bool hasAlpha);
    static void UpdateLayout(Noesis::Texture* texture, VkImageLayout layout);

    void* CreatePixelShader(const char* label, uint8_t shader, const void* spirv, uint32_t size);
    void ClearPixelShaders();

    void SafeReleaseTexture(Noesis::Texture* texture);
    void SafeReleaseRenderTarget(Noesis::RenderTarget* surface);
    void SafeReleaseBuffer(VkBuffer buffer, VkDeviceMemory memory);

private:
    /// From RenderDevice
    //@{
    const Noesis::DeviceCaps& GetCaps() const override;
    Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width,
        uint32_t height, uint32_t sampleCount, bool needsStencil) override;
    Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* label,
        Noesis::RenderTarget* surface) override;
    Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width, uint32_t height,
        uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data) override;
    void UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y,
        uint32_t width, uint32_t height, const void* data) override;
    void BeginOffscreenRender() override;
    void EndOffscreenRender() override;
    void BeginOnscreenRender() override;
    void EndOnscreenRender() override;
    void SetRenderTarget(Noesis::RenderTarget* surface) override;
    void BeginTile(Noesis::RenderTarget* surface, const Noesis::Tile& tile) override;
    void EndTile(Noesis::RenderTarget* surface) override;
    void ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles,
        uint32_t numTiles) override;
    void* MapVertices(uint32_t bytes) override;
    void UnmapVertices() override;
    void* MapIndices(uint32_t bytes) override;
    void UnmapIndices() override;
    void DrawBatch(const Noesis::Batch& batch) override;
    //@}

private:
    struct Page;
    struct Layout;
    struct Program;
    struct DynamicBuffer;

    void FillCaps(bool sRGB);
    void InvalidateStateCache();

    uint32_t FindMemoryType(uint32_t typeBits, uint32_t required) const;
    uint32_t FindMemoryType(uint32_t typeBits, uint32_t optimal, uint32_t required) const;
    VkFormat FindStencilFormat() const;

    Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width, uint32_t height,
        uint32_t levels, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage,
        VkMemoryPropertyFlags memFlags, VkImageAspectFlags aspect);

    Page* AllocatePage(DynamicBuffer& buffer);
    void DestroyPage(Page* page);
    void CreateBuffer(DynamicBuffer& buffer, const char* label, uint32_t size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags);
    void* MapBuffer(DynamicBuffer& buffer, uint32_t size);
    void DestroyBuffer(DynamicBuffer& buffer);
    void CreateBuffers();
    void DestroyBuffers();

    void CreateShaders();
    void DestroyShaders();

    void CreateLayout(uint32_t signature, Layout& layout);
    void CreateLayouts();
    void DestroyLayouts();

    void CreateTransferCommandPool();
    void DestroyTransferCommandPool();

    void CreateDescriptorPool();
    void DestroyDescriptorPool();

    void CreateSamplers();
    void DestroySamplers();

    VkRenderPass CreateRenderPass(VkSampleCountFlagBits samples, bool needsStencil);
    void DestroyRenderPasses();

    void SetStencilRef(uint32_t stencilRef);
    void BindPipeline(const Noesis::Batch& batch);
    void SetBuffers(const Noesis::Batch& batch);
    void UploadUniforms(uint32_t i, const Noesis::UniformData* data, uint32_t& hash,
        Noesis::BaseVector<uint32_t>& offsets);
    void FillBufferInfo(uint32_t i, const Noesis::UniformData* data, VkDescriptorSet set,
        Noesis::BaseVector<VkDescriptorBufferInfo>& buffers,
        Noesis::BaseVector<VkWriteDescriptorSet>& writes, uint32_t& binding);
    void FillImageInfo(Noesis::Texture* texture, uint8_t sampler, VkDescriptorSet set,
        Noesis::BaseVector<VkDescriptorImageInfo>& images,
        Noesis::BaseVector<VkWriteDescriptorSet>& writes, uint32_t& binding);
    void TextureHash(uint32_t& hash, Noesis::Texture* texture, uint8_t sampler);
    void BindDescriptors(const Noesis::Batch& batch, const Layout& layout);

    void CreatePipelines(const char* label, uint8_t shader, VkRenderPass renderPass,
        VkGraphicsPipelineCreateInfo& pipelineInfo, bool stereo, uint32_t custom);
    void CreatePipelines(const char* label, uint8_t shader, VkRenderPass renderPass,
        VkShaderModule psModule, VkPipelineLayout layout, VkSampleCountFlagBits sampleCount,
        uint32_t custom);
    void CreatePipelines(VkRenderPass renderPass, VkSampleCountFlagBits sampleCount);
    void DestroyPipelines();

    void ProcessPendingDestroys(bool force);
    void EnsureTransferCommands();
    void FlushTransferCommands();

    void ChangeLayout(VkCommandBuffer commands, Noesis::Texture* texture, VkImageLayout layout);

private:
    VkInstance mInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    VkRenderPass mActiveRenderPass = VK_NULL_HANDLE;
    uint32_t mQueueFamilyIndex = 0;

    VKFactory::QueueSubmitFunc mQueueSubmit = nullptr;

    uint64_t mTransferPoolFrameNumber = 0;
    VkCommandPool mTransferCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer mTransferCommands = VK_NULL_HANDLE;
    Noesis::Vector<Noesis::Texture*, 2> mUpdatedTextures;

    // Global funcs
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

    // Instance funcs
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;

    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

    // Device funcs
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkDestroyRenderPass vkDestroyRenderPass;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
    PFN_vkCreateShaderModule vkCreateShaderModule;
    PFN_vkDestroyShaderModule vkDestroyShaderModule;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkDestroyPipeline vkDestroyPipeline;
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
    PFN_vkResetDescriptorPool vkResetDescriptorPool;
    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
    PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
    PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkDestroyImage vkDestroyImage;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkDestroyImageView vkDestroyImageView;
    PFN_vkCreateSampler vkCreateSampler;
    PFN_vkDestroySampler vkDestroySampler;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
    PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkDestroyCommandPool vkDestroyCommandPool;
    PFN_vkResetCommandPool vkResetCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdResolveImage vkCmdResolveImage;
    PFN_vkQueueSubmit vkQueueSubmit;

    PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
    PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;

    bool mStereoSupport;
    Noesis::DeviceCaps mCaps;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    VkPhysicalDeviceFeatures mDeviceFeatures;
    VkPhysicalDeviceProperties mDeviceProperties;
    VkFormat mStencilFormat;
    VkFormat mBackBufferFormat;

    struct Page
    {
        uint32_t hash;
        VkBuffer buffer;
        VkDeviceMemory memory;
        void* base;
        uint64_t frameNumber;
        Page* next;
    };

    struct DynamicBuffer
    {
        uint32_t size;
        uint32_t pos;
        uint32_t drawPos;

        const char* label;

        Page* currentPage;
        Page* freePages;
        Page* pendingPages;

        uint32_t numPages;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags memFlags;
    };

    DynamicBuffer mVertices;
    DynamicBuffer mIndices;
    DynamicBuffer mConstants[4];
    DynamicBuffer mTexUpload;

    struct Layout
    {
        uint32_t signature;
        VkDescriptorSetLayout setLayout;
        VkPipelineLayout pipelineLayout;
    };

    Noesis::HashMap<uint32_t, Layout, 16> mLayoutMap;
    Noesis::HashMap<VkRenderPass, VkSampleCountFlagBits> mCachedPipelineRenderPasses;
    Noesis::HashMap<uint32_t, uint32_t> mPipelineMap;
    Noesis::Vector<VkPipeline> mPipelines;

    VkShaderModule mVertexShaders[Noesis::Shader::Vertex::Count];
    VkShaderModule mVertexShadersVR[Noesis::Shader::Vertex::Count];
    VkShaderModule mPixelShaders[Noesis::Shader::Count];
    Layout mLayouts[Noesis::Shader::Count];

    struct CustomShader
    {
        Noesis::String label;
        uint8_t id;
        Layout layout;
        VkShaderModule module;
    };

    Noesis::Vector<CustomShader> mCustomShaders;

    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    Noesis::Vector<Noesis::Pair<VkDescriptorPool, uint64_t>> mFreeDescriptorPools;
    Noesis::HashMap<uint32_t, VkDescriptorSet> mDescriptorSetMap;

    VkSampler mSamplers[64];
    VkRenderPass mRenderPasses[6][2] = {};

    uint64_t mFrameNumber;
    uint64_t mSafeFrameNumber;

    struct PendingDestroy
    {
        uint64_t frame;

        VkImage image;
        VkBuffer buffer;
        VkImageView view;
        VkDeviceMemory memory;

        VkFramebuffer framebuffer;
    };

    Noesis::Vector<PendingDestroy> mPendingDestroys;
    uint32_t mLastTextureHashValue = 1;
    uint32_t mLastBufferHashValue = 1;

    uint32_t mCachedStencilRef;
    uint32_t mCachedConstantHash[4];
    VkBuffer mCachedIndexBuffer;
    VkPipeline mCachedPipeline;
};

}

#endif
