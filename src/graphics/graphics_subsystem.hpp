// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "../vulkancore/device.hpp"
#include "../vulkancore/swapchain.hpp"

namespace graphics {
    class graphics_subsystem final : public subsystem {
    public:
        graphics_subsystem();
        ~graphics_subsystem() override;

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

        [[nodiscard]] auto get_width() const noexcept -> float { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> float { return m_height; }

    private:
        auto boot_vulkan_core() -> void;
        auto create_command_pool() -> void;
        auto create_semaphores() -> void;
        auto create_command_buffers() -> void;
        auto create_fences() -> void;
        auto setup_depth_stencil() -> void;
        auto setup_render_pass() -> void;
        auto setup_frame_buffer() -> void;
        auto create_pipeline_cache() -> void;
        auto recreate_swapchain() -> void;

        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::optional<vkb::device> m_device {};
        std::optional<vkb::swapchain> m_swapchain {};
        struct {
            vk::Semaphore present_complete {}; // Swap chain image presentation
            vk::Semaphore render_complete {}; // Command buffer submission and execution
        } m_semaphores {};
        vk::PipelineStageFlags m_submit_info_wait_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::CommandPool m_command_pool {};
        std::vector<vk::CommandBuffer> m_command_buffers {}; // Command buffers used for rendering
        std::vector<vk::Fence> m_wait_fences {}; // Wait fences to sync command buffer access
        vk::SubmitInfo m_submit_info {};
        struct {
            vk::Image image {};
            vk::ImageView view {};
            vk::DeviceMemory memory {};
        } m_depth_stencil {};
        vk::RenderPass m_render_pass {};
        std::vector<vk::Framebuffer> m_framebuffers {};
        vk::PipelineCache m_pipeline_cache {};
        std::uint32_t m_current_buffer = 0;
    };
}
