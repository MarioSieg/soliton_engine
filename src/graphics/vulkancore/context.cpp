// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "context.hpp"
#include "../shader.hpp"

#include "../../scripting/system_variable.hpp"

namespace lu::vkb {
    static const system_variable<bool> cv_enable_vulkan_validation_layers {
        "renderer.enable_vulkan_validation_layers",
        {false}
    };
    static const system_variable<bool> cv_enable_vsync {
        "renderer.force_vsync",
        {false}
    };

    context::context(const context_desc& desc) : m_window{desc.window}, m_concurrent_frames{desc.concurrent_frames} {
        passert(m_window != nullptr);

        switch (desc.msaa_samples) {
            case 0: m_msaa_samples = vk::SampleCountFlagBits::e1; break;
            case 2: m_msaa_samples = vk::SampleCountFlagBits::e2; break;
            case 4: m_msaa_samples = vk::SampleCountFlagBits::e4; break;
            case 8: m_msaa_samples = vk::SampleCountFlagBits::e8; break;
            default: panic("Invalid MSAA sample count: {}", desc.msaa_samples);
        }

        boot_vulkan_core();
        create_command_pools();
        create_command_buffers();
        create_sync_prims();
        setup_depth_stencil();
        create_msaa_target();
        setup_render_pass();
        setup_frame_buffer();
        create_pipeline_cache();

        m_descriptor_allocator.emplace();
        m_descriptor_layout_cache.emplace();
        m_shutdown_deletion_queue.push([this] {
            m_descriptor_allocator.reset();
            m_descriptor_layout_cache.reset();
        });
    }

    context::~context() {
        // Dump VMA Infos
        char* vma_stats_string = nullptr;
        vmaBuildStatsString(m_device->get_allocator(), &vma_stats_string, true);
        std::stringstream ss;
        ss.str(vma_stats_string);
        log_info("-------- VMA Stats --------");
        for (std::string line; std::getline(ss, line); ) {
            log_info("{}", line);
        }
        vmaFreeStatsString(m_device->get_allocator(), vma_stats_string);

        vkcheck(m_device->get_logical_device().waitIdle());
        m_shutdown_deletion_queue.flush();
    }

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
    auto context::begin_frame(
        const XMFLOAT4A& clear_color,
        vk::CommandBufferInheritanceInfo* out_inheritance_info
    ) -> eastl::optional<command_buffer> {
        m_clear_values[0].color = eastl::bit_cast<vk::ClearColorValue>(clear_color);
        m_clear_values[1].color = eastl::bit_cast<vk::ClearColorValue>(clear_color);
        m_clear_values[2].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        // Use a fence to wait until the command buffer has finished execution before using it again
        vkcheck(m_device->get_logical_device().waitForFences(1, &m_wait_fences[m_current_concurrent_frame_idx], vk::True, eastl::numeric_limits<std::uint64_t>::max()));
        vkcheck(m_device->get_logical_device().resetFences(1, &m_wait_fences[m_current_concurrent_frame_idx]));

        // Get the next swap chain image from the implementation
        // Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images/imageIndex on our own
        vk::Result result = m_swapchain->acquire_next_image(m_semaphores.present_complete[m_current_concurrent_frame_idx], m_image_index);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            if (result == vk::Result::eErrorOutOfDateKHR) {
                on_resize();
                return eastl::nullopt; // Skip rendering this frame
            }
        } else {
            vkcheck(result);
        }

        if (out_inheritance_info) {
            *out_inheritance_info = vk::CommandBufferInheritanceInfo {};
            out_inheritance_info->renderPass = m_scene_render_pass;
            out_inheritance_info->framebuffer = m_framebuffers[m_image_index];
        }

        const vk::CommandBuffer cmd_buf = m_command_buffers[m_current_concurrent_frame_idx];
        return command_buffer{m_graphics_command_pool, cmd_buf, dvc().get_graphics_queue(), vk::QueueFlagBits::eGraphics};
    }

    auto context::end_frame(command_buffer& cmd) -> void {
        vk::PipelineStageFlags wait_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submit_info {};
        submit_info.pWaitDstStageMask = &wait_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &*cmd;
        // Semaphore to wait upon before the submitted command buffer starts executing
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_semaphores.present_complete[m_current_concurrent_frame_idx];
        // Semaphore to be signaled when command buffers have completed
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &m_semaphores.render_complete[m_current_concurrent_frame_idx];

        // Submit to the graphics queue passing a wait fence
        vkcheck(cmd.queue().submit(1, &submit_info, m_wait_fences[m_current_concurrent_frame_idx]));

        // Present the current frame buffer to the swap chain
        // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
        // This ensures that the image is not presented to the windowing system until all commands have been submitted
        vk::PresentInfoKHR present_info {};
        present_info.swapchainCount = 1;
        vk::SwapchainKHR swapchain = m_swapchain->get_swapchain();
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &m_image_index;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_semaphores.render_complete[m_current_concurrent_frame_idx];
        vk::Result result = m_device->get_graphics_queue().presentKHR(&present_info);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            on_resize();
        } else {
            vkcheck(result);
        }

        m_current_concurrent_frame_idx = (m_current_concurrent_frame_idx + 1) % m_concurrent_frames; // Advance to the next frame
    }

    auto context::on_resize() -> void {
        vkcheck(m_device->get_logical_device().waitIdle());

        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        m_width = w;
        m_height = h;

        recreate_swapchain();

        destroy_depth_stencil();
        setup_depth_stencil();

        destroy_msaa_target();
        create_msaa_target();

        destroy_frame_buffer();
        setup_frame_buffer();

        destroy_command_buffers();
        create_command_buffers();

        destroy_sync_prims();
        create_sync_prims();

        m_shutdown_deletion_queue.flush();
        vkcheck(m_device->get_logical_device().waitIdle());
    }

    auto context::boot_vulkan_core() -> void {
        m_device.emplace(cv_enable_vulkan_validation_layers());
        m_swapchain.emplace(m_device->get_instance(), m_device->get_physical_device(), m_device->get_logical_device());
        m_swapchain->init_surface(m_window);
        recreate_swapchain();
        m_shutdown_deletion_queue.push([this] {
            m_swapchain.reset();
            m_device.reset();
        });
    }

    auto context::create_sync_prims() -> void {
        constexpr vk::SemaphoreCreateInfo semaphore_ci {};
        vk::FenceCreateInfo fence_ci {};
        fence_ci.flags = vk::FenceCreateFlagBits::eSignaled;

        m_semaphores.present_complete.resize(m_concurrent_frames);
        m_semaphores.render_complete.resize(m_concurrent_frames);
        m_wait_fences.resize(m_concurrent_frames);

        for (std::uint32_t i = 0; i < m_concurrent_frames; ++i) {
            vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, get_alloc(), &m_semaphores.present_complete[i]));
            vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, get_alloc(), &m_semaphores.render_complete[i]));
            vkcheck(m_device->get_logical_device().createFence(&fence_ci, get_alloc(), &m_wait_fences[i]));
        }

        m_shutdown_deletion_queue.push([this] {
            destroy_sync_prims();
        });
    }

    auto context::create_command_pools() -> void {
        const auto create_cp = [this](const std::uint32_t family, auto& dst) {
            vk::CommandPoolCreateInfo command_pool_ci {};
            command_pool_ci.queueFamilyIndex = family;
            command_pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            vkcheck(m_device->get_logical_device().createCommandPool(&command_pool_ci, get_alloc(), &dst));
        };
        create_cp(m_device->get_graphics_queue_idx(), m_graphics_command_pool);
        create_cp(m_device->get_compute_queue_idx(), m_compute_command_pool);
        create_cp(m_device->get_transfer_queue_idx(), m_transfer_command_pool);
        m_shutdown_deletion_queue.push([this] {
            m_device->get_logical_device().destroyCommandPool(m_graphics_command_pool, vkb::get_alloc());
            m_device->get_logical_device().destroyCommandPool(m_compute_command_pool, vkb::get_alloc());
            m_device->get_logical_device().destroyCommandPool(m_transfer_command_pool, vkb::get_alloc());
        });
    }

    auto context::create_command_buffers() -> void {
        m_command_buffers.resize(m_concurrent_frames);

        vk::CommandBufferAllocateInfo command_buffer_allocate_info {};
        command_buffer_allocate_info.commandPool = m_graphics_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_allocate_info.commandBufferCount = static_cast<std::uint32_t>(m_command_buffers.size());
        vkcheck(m_device->get_logical_device().allocateCommandBuffers(&command_buffer_allocate_info, m_command_buffers.data()));

        m_shutdown_deletion_queue.push([this] {
            destroy_command_buffers();
        });
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
        vkcheck(m_device->get_logical_device().createImageView(&image_view_ci, get_alloc(), &m_depth_stencil.view));

        m_shutdown_deletion_queue.push([this] {
            destroy_depth_stencil();
        });
    }

    // Render pass setup
    // Render passes are a new concept in Vulkan. They describe the attachments used during rendering and may contain multiple subpasses with attachment dependencies
    // This allows the driver to know up-front what the rendering will look like and is a good opportunity to optimize especially on tile-based renderers (with multiple subpasses)
    // Using sub pass dependencies also adds implicit layout transitions for the attachment used, so we don't need to add explicit image memory barriers to transform them
    auto context::setup_render_pass() -> void {
        eastl::array<vk::AttachmentDescription, 3> attachments {};

        // Multisampled attachment that we render to
        attachments[0].format = m_swapchain->get_format();
        attachments[0].samples = m_msaa_samples;
        attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[0].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].initialLayout = vk::ImageLayout::eUndefined;
        attachments[0].finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        // This is the frame buffer attachment to where the multisampled image
		// will be resolved to and which will be presented to the swapchain
        attachments[1].format = m_swapchain->get_format();
        attachments[1].samples = vk::SampleCountFlagBits::e1;
        attachments[1].loadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].storeOp = vk::AttachmentStoreOp::eStore;
        attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[1].initialLayout = vk::ImageLayout::eUndefined;
        attachments[1].finalLayout = vk::ImageLayout::ePresentSrcKHR;

        // Multisampled depth attachment we render to
        attachments[2].format = m_device->get_depth_format();
        attachments[2].samples = m_msaa_samples;
        attachments[2].loadOp = vk::AttachmentLoadOp::eClear;
        attachments[2].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[2].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachments[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachments[2].initialLayout = vk::ImageLayout::eUndefined;
        attachments[2].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_reference {};
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        // Resolve attachment reference for the color attachment
        vk::AttachmentReference resolve_reference {};
        resolve_reference.attachment = 1;
        resolve_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_reference {};
        depth_reference.attachment = 2;
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
        subpass_description.pResolveAttachments = &resolve_reference;

        // Subpass dependencies for layout transitions
        eastl::array<vk::SubpassDependency, 2> dependencies {};

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

        vkcheck(m_device->get_logical_device().createRenderPass(&render_pass_ci, get_alloc(), &m_scene_render_pass));
        vkcheck(m_device->get_logical_device().createRenderPass(&render_pass_ci, get_alloc(), &m_skybox_render_pass));
        vkcheck(m_device->get_logical_device().createRenderPass(&render_pass_ci, get_alloc(), &m_ui_render_pass));

        m_shutdown_deletion_queue.push([this] {
            m_device->get_logical_device().destroyRenderPass(m_scene_render_pass, vkb::get_alloc());
            m_device->get_logical_device().destroyRenderPass(m_ui_render_pass, vkb::get_alloc());
        });
    }

    auto context::setup_frame_buffer() -> void {
        // Create a frame buffer for every image in the swapchain
        m_framebuffers.resize(m_swapchain->get_image_count());
        for (std::size_t i = 0; i < m_framebuffers.size(); ++i) {
            eastl::array<vk::ImageView, 3> attachments {};
            attachments[0] = m_msaa_target.color.view;
            attachments[1] = m_swapchain->get_buffer(i).view;
            attachments[2] = m_msaa_target.depth.view;

            vk::FramebufferCreateInfo framebuffer_ci {};
            framebuffer_ci.renderPass = m_scene_render_pass;
            framebuffer_ci.attachmentCount = attachments.size();
            framebuffer_ci.pAttachments = attachments.data();
            framebuffer_ci.width = m_width;
            framebuffer_ci.height = m_height;
            framebuffer_ci.layers = 1;
            vkcheck(m_device->get_logical_device().createFramebuffer(&framebuffer_ci, get_alloc(), &m_framebuffers[i]));
        }

        m_shutdown_deletion_queue.push([this] {
            destroy_frame_buffer();
        });
    }

    auto context::create_pipeline_cache() -> void {
        constexpr vk::PipelineCacheCreateInfo pipeline_cache_ci {};
        vkcheck(m_device->get_logical_device().createPipelineCache(&pipeline_cache_ci, get_alloc(), &m_pipeline_cache));
    }

    auto context::recreate_swapchain() -> void {
        m_swapchain->create(m_width, m_height, cv_enable_vsync(), false);
    }

    auto context::create_msaa_target() -> void {
        // Color target:

        vk::ImageCreateInfo image_ci {};
        image_ci.imageType = vk::ImageType::e2D;
        image_ci.format = m_swapchain->get_format();
        image_ci.extent.width = m_width;
        image_ci.extent.height = m_height;
        image_ci.extent.depth = 1;
        image_ci.mipLevels = 1;
        image_ci.arrayLayers = 1;
        image_ci.samples = m_msaa_samples;
        image_ci.tiling = vk::ImageTiling::eOptimal;
        image_ci.sharingMode = vk::SharingMode::eExclusive;
        image_ci.initialLayout = vk::ImageLayout::eUndefined;
        image_ci.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;

        VmaAllocationCreateInfo vma_allocation_ci {};
        vma_allocation_ci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vma_allocation_ci.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        VmaAllocationInfo alloc_info {};

        vkccheck(vmaCreateImage(
            m_device->get_allocator(),
            &static_cast<VkImageCreateInfo&>(image_ci),
            &vma_allocation_ci,
            reinterpret_cast<VkImage*>(&m_msaa_target.color.image),
            &m_msaa_target.color.memory,
            &alloc_info
        ));

        vk::ImageViewCreateInfo image_view_ci {};
        image_view_ci.viewType = vk::ImageViewType::e2D;
        image_view_ci.format = m_swapchain->get_format(); // TODO get from swap chain
        image_view_ci.components.r = vk::ComponentSwizzle::eR;
        image_view_ci.components.g = vk::ComponentSwizzle::eG;
        image_view_ci.components.b = vk::ComponentSwizzle::eB;
        image_view_ci.components.a = vk::ComponentSwizzle::eA;
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_ci.subresourceRange.levelCount = 1;
        image_view_ci.subresourceRange.layerCount = 1;
        image_view_ci.image = m_msaa_target.color.image;

        vkcheck(m_device->get_logical_device().createImageView(&image_view_ci, get_alloc(), &m_msaa_target.color.view));

        // depth target
        image_ci.format = m_device->get_depth_format();
        image_ci.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment;
        vkccheck(vmaCreateImage(
            m_device->get_allocator(),
            &static_cast<VkImageCreateInfo&>(image_ci),
            &vma_allocation_ci,
            reinterpret_cast<VkImage*>(&m_msaa_target.depth.image),
            &m_msaa_target.depth.memory,
            &alloc_info
        ));

        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        image_view_ci.image = m_msaa_target.depth.image;
        image_view_ci.format = m_device->get_depth_format();
        if (image_ci.format >= vk::Format::eD16UnormS8Uint) {
            image_view_ci.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }

        vkcheck(m_device->get_logical_device().createImageView(&image_view_ci, get_alloc(), &m_msaa_target.depth.view));

        m_shutdown_deletion_queue.push([this] {
            destroy_msaa_target();
        });
    }

    auto context::destroy_depth_stencil() const -> void {
        m_device->get_logical_device().destroyImageView(m_depth_stencil.view, vkb::get_alloc());
        vmaDestroyImage(m_device->get_allocator(), m_depth_stencil.image, m_depth_stencil.memory);
    }

    auto context::destroy_msaa_target() const -> void {
        vmaDestroyImage(m_device->get_allocator(), m_msaa_target.color.image, m_msaa_target.color.memory);
        vmaDestroyImage(m_device->get_allocator(), m_msaa_target.depth.image, m_msaa_target.depth.memory);
        m_device->get_logical_device().destroyImageView(m_msaa_target.color.view, vkb::get_alloc());
        m_device->get_logical_device().destroyImageView(m_msaa_target.depth.view, vkb::get_alloc());
    }

    auto context::destroy_frame_buffer() const -> void {
        for (auto&& framebuffer : m_framebuffers) {
            m_device->get_logical_device().destroyFramebuffer(framebuffer, vkb::get_alloc());
        }
    }

    auto context::destroy_command_buffers() const -> void {
        m_device->get_logical_device().freeCommandBuffers(m_graphics_command_pool, m_concurrent_frames, m_command_buffers.data());
    }

    auto context::destroy_sync_prims() const -> void {
        for (auto&& fence : m_wait_fences) {
            m_device->get_logical_device().destroyFence(fence, vkb::get_alloc());
        }
        for (auto&& semaphore : m_semaphores.render_complete) {
            m_device->get_logical_device().destroySemaphore(semaphore, vkb::get_alloc());
        }
        for (auto&& semaphore : m_semaphores.present_complete) {
            m_device->get_logical_device().destroySemaphore(semaphore, vkb::get_alloc());
        }
    }

    auto context::begin_render_pass(command_buffer& cmd, vk::RenderPass pass, vk::SubpassContents contents) -> void {
        vk::RenderPassBeginInfo render_pass_begin_info {};
        render_pass_begin_info.renderPass = pass;
        render_pass_begin_info.framebuffer = m_framebuffers[m_image_index];
        render_pass_begin_info.renderArea.extent.width = m_width;
        render_pass_begin_info.renderArea.extent.height = m_height;
        render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(m_clear_values.size());
        render_pass_begin_info.pClearValues = m_clear_values.data();
        (*cmd).beginRenderPass(&render_pass_begin_info, contents);
    }

    auto context::end_render_pass(command_buffer& cmd) -> void {
        (*cmd).endRenderPass();
    }

    static constinit std::atomic_bool s_init;

    auto context::create(const context_desc& desc) -> void {
        if (s_init.load()) return;
        s_instance = new context{desc};
        s_init.store(true);
    }

    auto context::shutdown() -> void {
        if (!s_init.load()) return;
        delete s_instance;
        s_instance = nullptr;
        s_init.store(false);
    }

    auto context::compute_aligned_dynamic_ubo_size(const std::size_t size) noexcept -> std::size_t {
        return size;
        const std::size_t min_align = m_device->get_physical_device_props().limits.minUniformBufferOffsetAlignment;
        if (!min_align) return size;
        const std::size_t mask = min_align-1;
        return (size+mask) & ~mask;
    }
}
