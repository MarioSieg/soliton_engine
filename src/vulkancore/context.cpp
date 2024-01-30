// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "context.hpp"
#include "imgui_impl_vulkan.h"

#include "../graphics/imgui/imgui_impl_glfw.h"
#include "../graphics/imgui/font_awesome.ttf.inl"
#include "../graphics/imgui/jetbrains_mono.ttf.inl"
#include "../graphics/imgui/font_awesome_pro_5.hpp"

namespace vkb {
    context::context(GLFWwindow* window) : m_window{window} {
        passert(m_window != nullptr);
        boot_vulkan_core();
        create_command_pool();
        create_command_buffers();
        create_sync_prims();
        setup_depth_stencil();
        setup_render_pass();
        setup_frame_buffer();
        create_pipeline_cache();
        create_imgui_renderer();
    }

    context::~context() {
        m_device->get_logical_device().waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        // Dump VMA Infos
#if 0
        char* vma_stats_string = nullptr;
        vmaBuildStatsString(m_device->get_allocator(), &vma_stats_string, true);
        spdlog::info("VMA Stats:\n{}", vma_stats_string);
        vmaFreeStatsString(m_device->get_allocator(), vma_stats_string);
#endif

        m_device->get_logical_device().destroyDescriptorPool(m_imgui_descriptor_pool, &s_allocator);
        m_device->get_logical_device().destroyPipelineCache(m_pipeline_cache, &s_allocator);
        m_device->get_logical_device().destroyRenderPass(m_render_pass, &s_allocator);

        destroy_depth_stencil();
        destroy_frame_buffer();

        destroy_command_buffers();

        m_device->get_logical_device().destroyCommandPool(m_command_pool, &s_allocator);

        destroy_sync_prims();

        m_swapchain.reset();
        m_device.reset();
    }

    auto context::begin_frame(const DirectX::XMFLOAT4& clear_color) -> vk::CommandBuffer {

        // Use a fence to wait until the command buffer has finished execution before using it again
        vkcheck(m_device->get_logical_device().waitForFences(1, &m_wait_fences[m_current_frame], vk::True, std::numeric_limits<std::uint64_t>::max()));
        vkcheck(m_device->get_logical_device().resetFences(1, &m_wait_fences[m_current_frame]));

        // Get the next swap chain image from the implementation
        // Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images/imageIndex on our own
        vk::Result result = m_swapchain->acquire_next_image(m_semaphores.present_complete[m_current_frame], m_image_index);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            if (result == vk::Result::eErrorOutOfDateKHR) {
                on_resize();
                return nullptr; // Skip rendering this frame
            }
        } else {
            vkcheck(result);
        }

        constexpr vk::CommandBufferBeginInfo command_buffer_begin_info {};

        // Set clear values for all framebuffer attachments with loadOp set to clear
        // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
        std::array<vk::ClearValue, 2> clear_values {};
        clear_values[0].color = std::bit_cast<vk::ClearColorValue>(clear_color);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        vk::RenderPassBeginInfo render_pass_begin_info {};
        render_pass_begin_info.renderPass = m_render_pass;
        render_pass_begin_info.framebuffer = m_framebuffers[m_image_index];
        render_pass_begin_info.renderArea.extent.width = m_width;
        render_pass_begin_info.renderArea.extent.height = m_height;
        render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());
        render_pass_begin_info.pClearValues = clear_values.data();

        const vk::CommandBuffer cmd_buf = m_command_buffers[m_current_frame];
        cmd_buf.reset({});
        vkcheck(cmd_buf.begin(&command_buffer_begin_info));

        // Start the first sub pass specified in our default render pass setup by the base class
        // This will clear the color and depth attachment
        cmd_buf.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);

        const auto w = static_cast<float>(m_width);
        const auto h = static_cast<float>(m_height);

        // Update dynamic viewport state
        vk::Viewport viewport {};
        viewport.width = w;
        viewport.height = -h;
        viewport.x = 0.0f;
        viewport.y = h;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmd_buf.setViewport(0, 1, &viewport);

        // Update dynamic scissor state
        vk::Rect2D scissor {};
        scissor.extent.width = m_width;
        scissor.extent.height = m_height;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        cmd_buf.setScissor(0, 1, &scissor);

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();

        return cmd_buf;
    }

    auto context::end_frame(vk::CommandBuffer cmd_buf) -> void {
        passert(cmd_buf);

        cmd_buf.endRenderPass();
        cmd_buf.end();

        vk::PipelineStageFlags wait_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submit_info {};
        submit_info.pWaitDstStageMask = &wait_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buf;
        // Semaphore to wait upon before the submitted command buffer starts executing
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_semaphores.present_complete[m_current_frame];
        // Semaphore to be signaled when command buffers have completed
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &m_semaphores.render_complete[m_current_frame];

        // Submit to the graphics queue passing a wait fence
        vkcheck(m_device->get_graphics_queue().submit(1, &submit_info, m_wait_fences[m_current_frame]));

        // Present the current frame buffer to the swap chain
        // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
        // This ensures that the image is not presented to the windowing system until all commands have been submitted
        vk::PresentInfoKHR present_info {};
        present_info.swapchainCount = 1;
        vk::SwapchainKHR swapchain = m_swapchain->get_swapchain();
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &m_image_index;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_semaphores.render_complete[m_current_frame];
        vk::Result result = m_device->get_graphics_queue().presentKHR(&present_info);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            on_resize();
        } else {
            vkcheck(result);
        }

        m_current_frame = (m_current_frame + 1) % k_max_concurrent_frames;
    }

    auto context::render_imgui(ImDrawData* data, vk::CommandBuffer cmd_buf) -> void {
        assert(data);
        ImGui_ImplVulkan_RenderDrawData(data, cmd_buf);
    }

    auto context::on_resize() -> void {
        m_device->get_logical_device().waitIdle();

        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        m_width = w;
        m_height = h;

        recreate_swapchain();

        destroy_depth_stencil();
        setup_depth_stencil();

        destroy_frame_buffer();
        setup_frame_buffer();

        destroy_command_buffers();
        create_command_buffers();

        destroy_sync_prims();
        create_sync_prims();

        m_device->get_logical_device().waitIdle();
    }

    auto context::boot_vulkan_core() -> void {
        m_device.emplace(true);
        m_swapchain.emplace(m_device->get_instance(), m_device->get_physical_device(), m_device->get_logical_device());
        m_swapchain->init_surface(m_window);
        recreate_swapchain();
    }

    auto context::create_sync_prims() -> void {
        constexpr vk::SemaphoreCreateInfo semaphore_ci {};
        vk::FenceCreateInfo fence_ci {};
        fence_ci.flags = vk::FenceCreateFlagBits::eSignaled;
        for (std::uint32_t i = 0; i < k_max_concurrent_frames; ++i) {
            vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, &s_allocator, &m_semaphores.present_complete[i]));
            vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, &s_allocator, &m_semaphores.render_complete[i]));
            vkcheck(m_device->get_logical_device().createFence(&fence_ci, &s_allocator, &m_wait_fences[i]));
        }
    }

    auto context::create_command_pool() -> void {
        vk::CommandPoolCreateInfo command_pool_ci {};
        command_pool_ci.queueFamilyIndex = m_swapchain->get_queue_node_index();
        command_pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        vkcheck(m_device->get_logical_device().createCommandPool(&command_pool_ci, &s_allocator, &m_command_pool));
    }

    auto context::create_command_buffers() -> void {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info {};
        command_buffer_allocate_info.commandPool = m_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_allocate_info.commandBufferCount = static_cast<std::uint32_t>(m_command_buffers.size());
        vkcheck(m_device->get_logical_device().allocateCommandBuffers(&command_buffer_allocate_info, m_command_buffers.data()));
    }

    auto context::setup_depth_stencil() -> void {
        // Create an optimal image used as the depth stencil attachment
        vk::ImageCreateInfo image_ci {};
        image_ci.imageType = vk::ImageType::e2D;
        image_ci.format = m_device->get_depth_format();
        image_ci.extent.width = m_width;
        image_ci.extent.height = m_height;
        image_ci.extent.depth = 1;
        image_ci.mipLevels = 1;
        image_ci.arrayLayers = 1;
        image_ci.samples = vk::SampleCountFlagBits::e1;
        image_ci.tiling = vk::ImageTiling::eOptimal;
        image_ci.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        image_ci.initialLayout = vk::ImageLayout::eUndefined;

        VmaAllocationCreateInfo vma_allocation_ci {};
        vma_allocation_ci.usage = VMA_MEMORY_USAGE_AUTO;
        vma_allocation_ci.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaCreateImage(
            m_device->get_allocator(),
            &static_cast<VkImageCreateInfo&>(image_ci),
            &vma_allocation_ci,
            reinterpret_cast<VkImage*>(&m_depth_stencil.image),
            &m_depth_stencil.memory,
            nullptr
        );

        // Create a view for the depth stencil image
        // Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
        // This allows for multiple views of one image with differing ranges (e.g. for different layers)
        vk::ImageViewCreateInfo image_view_ci {};
        image_view_ci.viewType = vk::ImageViewType::e2D;
        image_view_ci.format = m_device->get_depth_format();
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        image_view_ci.subresourceRange.levelCount = 1;
        image_view_ci.subresourceRange.layerCount = 1;
        image_view_ci.subresourceRange.baseArrayLayer = 0;
        image_view_ci.subresourceRange.baseMipLevel = 0;
        image_view_ci.image = m_depth_stencil.image;
        // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
        if (m_device->get_depth_format() >= vk::Format::eD16UnormS8Uint) {
            image_view_ci.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
        vkcheck(m_device->get_logical_device().createImageView(&image_view_ci, &s_allocator, &m_depth_stencil.view));
    }

    // Render pass setup
    // Render passes are a new concept in Vulkan. They describe the attachments used during rendering and may contain multiple subpasses with attachment dependencies
    // This allows the driver to know up-front what the rendering will look like and is a good opportunity to optimize especially on tile-based renderers (with multiple subpasses)
    // Using sub pass dependencies also adds implicit layout transitions for the attachment used, so we don't need to add explicit image memory barriers to transform them
    auto context::setup_render_pass() -> void {
        std::array<vk::AttachmentDescription, 2> attachments {};

        // Color attachment
        attachments[0].format = m_swapchain->get_format();
        attachments[0].samples = vk::SampleCountFlagBits::e1;
        attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
        attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].initialLayout = vk::ImageLayout::eUndefined;
        attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

        // Depth attachment
        attachments[1].format = m_device->get_depth_format();
        attachments[1].samples = vk::SampleCountFlagBits::e1;
        attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].initialLayout = vk::ImageLayout::eUndefined;
        attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_reference {};
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_reference {};
        depth_reference.attachment = 1;
        depth_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription subpass_description {};
        subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &color_reference;
        subpass_description.pDepthStencilAttachment = &depth_reference;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = nullptr;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = nullptr;
        subpass_description.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<vk::SubpassDependency, 2> dependencies {};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

        dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;

        vk::RenderPassCreateInfo render_pass_ci {};
        render_pass_ci.attachmentCount = static_cast<std::uint32_t>(attachments.size());
        render_pass_ci.pAttachments = attachments.data();
        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass_description;
        render_pass_ci.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_ci.pDependencies = dependencies.data();
        vkcheck(m_device->get_logical_device().createRenderPass(&render_pass_ci, &s_allocator, &m_render_pass));
    }

    auto context::setup_frame_buffer() -> void {
        // Create a frame buffer for every image in the swapchain
        m_framebuffers.resize(m_swapchain->get_image_count());
        for (std::size_t i = 0; i < m_framebuffers.size(); ++i) {
            std::array<vk::ImageView, 2> attachments {};
            attachments[0] = m_swapchain->get_buffer(i).view;
            attachments[1] = m_depth_stencil.view;

            vk::FramebufferCreateInfo framebuffer_ci {};
            framebuffer_ci.renderPass = m_render_pass;
            framebuffer_ci.attachmentCount = 2;
            framebuffer_ci.pAttachments = attachments.data();
            framebuffer_ci.width = m_width;
            framebuffer_ci.height = m_height;
            framebuffer_ci.layers = 1;
            vkcheck(m_device->get_logical_device().createFramebuffer(&framebuffer_ci, &s_allocator, &m_framebuffers[i]));
        }
    }

    auto context::create_pipeline_cache() -> void {
        constexpr vk::PipelineCacheCreateInfo pipeline_cache_ci {};
        vkcheck(m_device->get_logical_device().createPipelineCache(&pipeline_cache_ci, &s_allocator, &m_pipeline_cache));
    }

    auto context::recreate_swapchain() -> void {
        m_swapchain->create(m_width, m_height, false, false);
    }

    auto context::create_imgui_renderer() -> void {
        ImGui_ImplGlfw_InitForVulkan(m_window, true);

        constexpr std::size_t k_num = 1024;
        constexpr std::array<vk::DescriptorPoolSize, 11> k_pool_sizes {
            vk::DescriptorPoolSize { vk::DescriptorType::eSampler, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eSampledImage, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageImage, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformTexelBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageTexelBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eUniformBufferDynamic, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eStorageBufferDynamic, k_num },
            vk::DescriptorPoolSize { vk::DescriptorType::eInputAttachment, k_num }
        };

        vk::DescriptorPoolCreateInfo pool_info = {};
        pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_info.maxSets = k_num;
        pool_info.poolSizeCount = k_pool_sizes.size();
        pool_info.pPoolSizes = k_pool_sizes.data();

        vkcheck(m_device->get_logical_device().createDescriptorPool(&pool_info, &s_allocator, &m_imgui_descriptor_pool));

        ImGui_ImplVulkan_InitInfo init_info {};
        init_info.Instance = m_device->get_instance();
        init_info.PhysicalDevice = m_device->get_physical_device();
        auto dev = m_device->get_logical_device();
        init_info.Device = dev;
        init_info.QueueFamily = m_swapchain->get_queue_node_index();
        init_info.Queue = m_device->get_graphics_queue();
        init_info.PipelineCache = m_pipeline_cache;
        init_info.DescriptorPool = m_imgui_descriptor_pool;
        init_info.ImageCount = k_max_concurrent_frames;
        init_info.MinImageCount = k_max_concurrent_frames;
        init_info.Allocator = reinterpret_cast<const VkAllocationCallbacks*>(&s_allocator);
        passert(ImGui_ImplVulkan_Init(&init_info, m_render_pass));

        float font_size = 18.0f;

        // add primary text font:
        ImFontConfig config { };
        config.FontDataOwnedByAtlas = false;
        config.MergeMode = false;
        auto& io = ImGui::GetIO();
        ImFont* primaryFont = io.Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(k_ttf_jet_brains_mono)),
            sizeof(k_ttf_jet_brains_mono) / sizeof(*k_ttf_jet_brains_mono),
            font_size, // font size
            &config,
            io.Fonts->GetGlyphRangesDefault()
        );

        // now add font awesome icons:
        config.MergeMode = true;
        config.DstFont = primaryFont;
        struct font_range final {
            std::span<const std::uint8_t> data {};
            std::array<char16_t, 3> ranges {};
        };
        static constexpr font_range range = { k_font_awesome_ttf, { ICON_MIN_FA, ICON_MAX_FA, 0 } };
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<void*>(static_cast<const void*>(range.data.data())),
            static_cast<int>(range.data.size()),
            font_size-3.0f,
            &config,
            reinterpret_cast<const ImWchar*>(range.ranges.data())
        );
        static_assert(sizeof(ImWchar) == sizeof(char16_t));
        passert(ImGui_ImplVulkan_CreateFontsTexture());
    }

    auto context::destroy_depth_stencil() const -> void {
        m_device->get_logical_device().destroyImageView(m_depth_stencil.view, &s_allocator);
        vmaDestroyImage(m_device->get_allocator(), m_depth_stencil.image, m_depth_stencil.memory);
    }

    auto context::destroy_frame_buffer() const -> void {
        for (auto&& framebuffer : m_framebuffers) {
            m_device->get_logical_device().destroyFramebuffer(framebuffer, &s_allocator);
        }
    }

    auto context::destroy_command_buffers() const -> void {
        m_device->get_logical_device().freeCommandBuffers(m_command_pool, k_max_concurrent_frames, m_command_buffers.data());
    }

    auto context::destroy_sync_prims() const -> void {
        for (auto&& fence : m_wait_fences) {
            m_device->get_logical_device().destroyFence(fence, &s_allocator);
        }
        for (auto&& semaphore : m_semaphores.render_complete) {
            m_device->get_logical_device().destroySemaphore(semaphore, &s_allocator);
        }
        for (auto&& semaphore : m_semaphores.present_complete) {
            m_device->get_logical_device().destroySemaphore(semaphore, &s_allocator);
        }
    }
}
