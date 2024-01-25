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
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
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
        auto create_sync_prims() -> void;
        auto create_command_pool() -> void;
        auto create_command_buffers() -> void;
        auto setup_depth_stencil() -> void;
        auto setup_render_pass() -> void;
        auto setup_frame_buffer() -> void;
        auto create_pipeline_cache() -> void;
        auto recreate_swapchain() -> void;

        auto destroy_depth_stencil() -> void;
        auto destroy_frame_buffer() -> void;
        auto destroy_command_buffers() -> void;
        auto destroy_sync_prims() -> void;

        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::optional<vkb::device> m_device {};
        std::optional<vkb::swapchain> m_swapchain {};
        struct {
            std::array<vk::Semaphore, k_max_concurrent_frames> present_complete {}; // Swap chain image presentation
            std::array<vk::Semaphore, k_max_concurrent_frames> render_complete {}; // Command buffer submission and execution
        } m_semaphores {}; // Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering
        vk::PipelineStageFlags m_submit_info_wait_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::CommandPool m_command_pool {};
        std::array<vk::CommandBuffer, k_max_concurrent_frames> m_command_buffers {}; // Command buffers used for rendering
        std::array<vk::Fence, k_max_concurrent_frames> m_wait_fences {}; // Wait fences to sync command buffer access
        struct {
            vk::Image image {};
            vk::ImageView view {};
            vk::DeviceMemory memory {};
        } m_depth_stencil {};
        vk::RenderPass m_render_pass {};
        std::vector<vk::Framebuffer> m_framebuffers {};
        vk::PipelineCache m_pipeline_cache {};
        std::uint32_t m_current_frame = 0; // To select the correct sync objects, we need to keep track of the current frame
    };
}
