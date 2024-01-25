// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include <optional>

#include "device.hpp"
#include "swapchain.hpp"
#include "../math/DirectXMath.h"

namespace vkb {
    class context final : public no_copy, public no_move {
    public:
        inline constinit static std::unique_ptr<context> s_instance {};
        explicit context(
            GLFWwindow* window
        );
        ~context();

        [[nodiscard]] auto get_width() const noexcept -> std::uint32_t { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> std::uint32_t { return m_height; }
        [[nodiscard]] auto get_device() const noexcept -> const device& { return *m_device; }
        [[nodiscard]] auto get_swapchain() const noexcept -> const swapchain& { return *m_swapchain; }

        HOTPROC auto begin_frame(const DirectX::XMFLOAT4& clear_color) -> vk::CommandBuffer;
        HOTPROC auto end_frame(vk::CommandBuffer cmd_buf) -> void;
        auto on_resize() -> void;

    private:
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
        auto boot_vulkan_core() -> void;
        auto create_sync_prims() -> void;
        auto create_command_pool() -> void;
        auto create_command_buffers() -> void;
        auto setup_depth_stencil() -> void;
        auto setup_render_pass() -> void;
        auto setup_frame_buffer() -> void;
        auto create_pipeline_cache() -> void;
        auto recreate_swapchain() -> void;

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
    };
}
