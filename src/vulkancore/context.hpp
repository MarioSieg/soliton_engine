// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "device.hpp"
#include "swapchain.hpp"

#include "../graphics/imgui/imgui.h"
#include "../math/DirectXMath.h"

namespace vkb {
    class context final : public no_copy, public no_move {
    public:
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
        inline constinit static std::unique_ptr<context> s_instance {};
        explicit context(
            GLFWwindow* window
        );
        ~context();

        [[nodiscard]] auto get_width() const noexcept -> std::uint32_t { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> std::uint32_t { return m_height; }
        [[nodiscard]] auto get_device() const noexcept -> const device& { return *m_device; }
        [[nodiscard]] auto get_swapchain() const noexcept -> const swapchain& { return *m_swapchain; }
        [[nodiscard]] auto get_command_pool() const noexcept -> vk::CommandPool { return m_command_pool; }
        [[nodiscard]] auto get_command_buffers() const noexcept -> std::span<const vk::CommandBuffer> { return m_command_buffers; }
        [[nodiscard]] auto get_current_frame() const noexcept -> std::uint32_t { return m_current_frame; }
        [[nodiscard]] auto get_image_index() const noexcept -> std::uint32_t { return m_image_index; }
        [[nodiscard]] auto get_pipeline_cache() const noexcept -> vk::PipelineCache { return m_pipeline_cache; }
        [[nodiscard]] auto get_imgui_descriptor_pool() const noexcept -> vk::DescriptorPool { return m_imgui_descriptor_pool; }
        [[nodiscard]] auto get_render_pass() const noexcept -> vk::RenderPass { return m_render_pass; }
        [[nodiscard]] auto get_framebuffers() const noexcept -> std::span<const vk::Framebuffer> { return m_framebuffers; }
        [[nodiscard]] auto get_swapchain_image() const noexcept -> vk::Image { return m_swapchain->get_images()[m_image_index]; }
        [[nodiscard]] auto get_swapchain_image_view() const noexcept -> vk::ImageView { return m_swapchain->get_buffer(m_image_index).view; }
        [[nodiscard]] auto get_swapchain_image_format() const noexcept -> vk::Format { return m_swapchain->get_format(); }
        [[nodiscard]] auto get_swapchain_image_color_space() const noexcept -> vk::ColorSpaceKHR { return m_swapchain->get_color_space(); }
        [[nodiscard]] auto get_swapchain_image_count() const noexcept -> std::uint32_t { return m_swapchain->get_image_count(); }
        [[nodiscard]] auto get_swapchain_image_extent() const noexcept -> vk::Extent2D { return { m_width, m_height }; }
        [[nodiscard]] auto get_swapchain_image_aspect_ratio() const noexcept -> float { return static_cast<float>(m_width) / static_cast<float>(m_height); }
        [[nodiscard]] auto get_swapchain_image_viewport() const noexcept -> vk::Viewport { return { 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f }; }
        [[nodiscard]] auto get_swapchain_image_scissor() const noexcept -> vk::Rect2D { return { { 0, 0 }, { m_width, m_height } }; }
        [[nodiscard]] auto get_swapchain_image_depth_stencil_view() const noexcept -> vk::ImageView { return m_depth_stencil.view; }

        HOTPROC auto begin_frame(const DirectX::XMFLOAT4& clear_color) -> vk::CommandBuffer;
        HOTPROC auto end_frame(vk::CommandBuffer cmd_buf) -> void;
        auto render_imgui(ImDrawData* data, vk::CommandBuffer cmd_buf) -> void;
        auto on_resize() -> void;

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
        auto create_imgui_renderer() -> void;

        auto destroy_depth_stencil() const -> void;
        auto destroy_frame_buffer() const -> void;
        auto destroy_command_buffers() const -> void;
        auto destroy_sync_prims() const -> void;

        GLFWwindow* m_window = nullptr;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::optional<device> m_device;
        std::optional<swapchain> m_swapchain;
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
            VmaAllocation memory {};
        } m_depth_stencil {};
        vk::RenderPass m_render_pass {};
        std::vector<vk::Framebuffer> m_framebuffers {};
        vk::PipelineCache m_pipeline_cache {};
        std::uint32_t m_current_frame = 0; // To select the correct sync objects, we need to keep track of the current frame
        std::uint32_t m_image_index = 0; // The current swap chain image index
        vk::DescriptorPool m_imgui_descriptor_pool {};
    };

    // Convenience macros hehe
    #define vkb_context() (*vkb::context::s_instance)
    #define vkb_device() (vkb_context().get_device())
    #define vkb_swapchain() (vkb_context().get_swapchain())
    #define vkb_vk_device() (vkb_device().get_logical_device())
}
