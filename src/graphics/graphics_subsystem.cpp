// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"

#include <execution>
#include <mimalloc.h>

#include "imgui/text_editor.hpp"
#include "imgui/implot.h"

using platform::platform_subsystem;

namespace graphics {
    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");

        boot_vulkan_core();
        create_command_pool();
        create_semaphores();
        create_command_buffers();
        create_fences();
        setup_depth_stencil();
        setup_render_pass();
        setup_frame_buffer();
        create_pipeline_cache();

        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
    }

    graphics_subsystem::~graphics_subsystem() {
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        m_device->get_logical_device().destroyPipelineCache(m_pipeline_cache);
        m_device->get_logical_device().destroyRenderPass(m_render_pass);
        for (auto&& framebuffer : m_framebuffers) {
            m_device->get_logical_device().destroyFramebuffer(framebuffer);
        }
        m_device->get_logical_device().destroyImage(m_depth_stencil.image);
        m_device->get_logical_device().destroyImageView(m_depth_stencil.view);
        m_device->get_logical_device().freeMemory(m_depth_stencil.memory);
        for (auto&& fence : m_wait_fences) {
            m_device->get_logical_device().destroyFence(fence);
        }
        for (auto&& buffer : m_command_buffers) {
            m_device->get_logical_device().freeCommandBuffers(m_command_pool, buffer);
        }
        m_device->get_logical_device().destroyCommandPool(m_command_pool);
        m_device->get_logical_device().destroySemaphore(m_semaphores.render_complete);
        m_device->get_logical_device().destroySemaphore(m_semaphores.present_complete);
        m_swapchain.reset();
        m_device.reset();
    }

    [[nodiscard]] static auto get_main_camera() -> entity {
        const auto& scene = scene::get_active();
        if (scene) [[likely]] {
            auto query = scene->filter<const c_transform, c_camera>();
            if (query.count() > 0) {
                return query.first();
            }
        }
        return entity::null();
    }

    static XMMATRIX m_mtx_view;
    static XMMATRIX m_mtx_proj;
    static XMMATRIX m_mtx_view_proj;
    static c_transform m_camera_transform;

    static auto update_main_camera(float width, float height) -> void {
        entity main_cam = get_main_camera();
        if (!main_cam.is_valid() || !main_cam.is_alive()) [[unlikely]] {
            log_warn("No camera found in scene");
            return;
        }
        c_camera::active_camera = main_cam;
        m_camera_transform = *main_cam.get<c_transform>();
        c_camera& cam = *main_cam.get_mut<c_camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        m_mtx_view = cam.compute_view(m_camera_transform);
        m_mtx_proj = cam.compute_projection();
        m_mtx_view_proj = XMMatrixMultiply(m_mtx_view, m_mtx_proj);
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        m_width = static_cast<float>(w);
        m_height = static_cast<float>(h);
        update_main_camera(m_width, m_height);
        //ImGui::NewFrame();
        //auto& io = ImGui::GetIO();
        //io.DisplaySize.x = m_width;
        //io.DisplaySize.y = m_height;
        const vk::Result result = m_swapchain->acquire_next_image(m_semaphores.present_complete, m_current_buffer);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            //recreate_swapchain();
        } else {
            vkcheck(result);
        }
        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        const vk::Result result = m_swapchain->queue_present(m_device->get_graphics_queue(), m_current_buffer, m_semaphores.render_complete);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) [[unlikely]] {
            //recreate_swapchain();
        } else {
            vkcheck(result);
        }
        m_device->get_graphics_queue().waitIdle();
        //ImGui::EndFrame();
        c_camera::active_camera = entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {
        recreate_swapchain();
    }

    void graphics_subsystem::on_start(scene& scene) {
        entity camera = scene.spawn("MainCamera");
        camera.add<c_camera>();
    }

    auto graphics_subsystem::boot_vulkan_core() -> void {
        GLFWwindow* window = platform_subsystem::get_glfw_window();
        passert(window != nullptr);
        m_device.emplace(true);
        m_swapchain.emplace(m_device->get_instance(), m_device->get_physical_device(), m_device->get_logical_device());
        m_swapchain->init_surface(window);
        recreate_swapchain();
    }

    auto graphics_subsystem::create_command_pool() -> void {
        vk::CommandPoolCreateInfo command_pool_ci {};
        command_pool_ci.queueFamilyIndex = m_swapchain->get_queue_node_index();
        command_pool_ci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        vkcheck(m_device->get_logical_device().createCommandPool(&command_pool_ci, nullptr, &m_command_pool));
    }

    auto graphics_subsystem::create_semaphores() -> void {
        constexpr vk::SemaphoreCreateInfo semaphore_ci {};
        vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, nullptr, &m_semaphores.present_complete));
        vkcheck(m_device->get_logical_device().createSemaphore(&semaphore_ci, nullptr, &m_semaphores.render_complete));
        m_submit_info.pWaitDstStageMask = &m_submit_info_wait_stage_mask;
        m_submit_info.waitSemaphoreCount = 1;
        m_submit_info.pWaitSemaphores = &m_semaphores.present_complete;
        m_submit_info.signalSemaphoreCount = 1;
        m_submit_info.pSignalSemaphores = &m_semaphores.render_complete;
    }

    auto graphics_subsystem::create_command_buffers() -> void {
        m_command_buffers.resize(m_swapchain->get_image_count());
        vk::CommandBufferAllocateInfo command_buffer_allocate_info {};
        command_buffer_allocate_info.commandPool = m_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_allocate_info.commandBufferCount = static_cast<std::uint32_t>(m_command_buffers.size());
        vkcheck(m_device->get_logical_device().allocateCommandBuffers(&command_buffer_allocate_info, m_command_buffers.data()));
    }

    auto graphics_subsystem::create_fences() -> void {
        // Wait fences to sync command buffer access
        vk::FenceCreateInfo fence_ci { vk::FenceCreateFlagBits::eSignaled };
        m_wait_fences.resize(m_command_buffers.size());
        for (auto& fence : m_wait_fences) {
            vkcheck(m_device->get_logical_device().createFence(&fence_ci, nullptr, &fence));
        }
    }

    auto graphics_subsystem::setup_depth_stencil() -> void {
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
        vkcheck(m_device->get_logical_device().createImage(&image_ci, nullptr, &m_depth_stencil.image));

        vk::MemoryRequirements memory_requirements {};
        m_device->get_logical_device().getImageMemoryRequirements(m_depth_stencil.image, &memory_requirements);

        vk::MemoryAllocateInfo memory_allocate_info {};
        memory_allocate_info.allocationSize = memory_requirements.size;
        vk::Bool32 found = false;
        memory_allocate_info.memoryTypeIndex = m_device->get_mem_type(memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, found);
        passert(found);
        vkcheck(m_device->get_logical_device().allocateMemory(&memory_allocate_info, nullptr, &m_depth_stencil.memory));
        m_device->get_logical_device().bindImageMemory(m_depth_stencil.image, m_depth_stencil.memory, 0);

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
        vkcheck(m_device->get_logical_device().createImageView(&image_view_ci, nullptr, &m_depth_stencil.view));
    }

    auto graphics_subsystem::setup_render_pass() -> void {
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
        vkcheck(m_device->get_logical_device().createRenderPass(&render_pass_ci, nullptr, &m_render_pass));
    }

    auto graphics_subsystem::setup_frame_buffer() -> void {
        std::array<vk::ImageView, 2> attachments {};
        attachments[1] = m_depth_stencil.view;

        vk::FramebufferCreateInfo framebuffer_ci {};
        framebuffer_ci.renderPass = m_render_pass;
        framebuffer_ci.attachmentCount = 2;
        framebuffer_ci.pAttachments = attachments.data();
        framebuffer_ci.width = m_width;
        framebuffer_ci.height = m_height;
        framebuffer_ci.layers = 1;
        m_framebuffers.resize(m_swapchain->get_image_count());
        for (std::size_t i = 0; i < m_framebuffers.size(); ++i) {
            attachments[0] = m_swapchain->get_buffer(static_cast<std::uint32_t>(i)).view;
            vkcheck(m_device->get_logical_device().createFramebuffer(&framebuffer_ci, nullptr, &m_framebuffers[i]));
        }
    }

    auto graphics_subsystem::create_pipeline_cache() -> void {
        vk::PipelineCacheCreateInfo pipeline_cache_ci {};
        vkcheck(m_device->get_logical_device().createPipelineCache(&pipeline_cache_ci, nullptr, &m_pipeline_cache));
    }

    auto graphics_subsystem::recreate_swapchain() -> void {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        m_width = w;
        m_height = h;
        m_swapchain->create(m_width, m_height, false, false);
    }
}
