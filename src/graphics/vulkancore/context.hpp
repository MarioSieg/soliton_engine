// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "device.hpp"
#include "swapchain.hpp"
#include "command_buffer.hpp"
#include "descriptor.hpp"
#include "deletion_queue.hpp"

#include <DirectXMath.h>

namespace lu::graphics {
    class graphics_subsystem;
}

namespace lu::vkb {
    struct context_desc final {
        GLFWwindow* window = nullptr;
        std::uint32_t concurrent_frames = 3;
        std::uint32_t msaa_samples = 4;
    };

    class context final : public no_copy, public no_move {
    public:
        explicit context(const context_desc& desc);
        ~context();

        [[nodiscard]] auto get_width() const noexcept -> std::uint32_t { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> std::uint32_t { return m_height; }
        [[nodiscard]] auto get_device() const noexcept -> const device& { return *m_device; }
        [[nodiscard]] auto get_swapchain() const noexcept -> const swapchain& { return *m_swapchain; }
        [[nodiscard]] auto get_graphics_command_pool() const noexcept -> vk::CommandPool { return m_graphics_command_pool; }
        [[nodiscard]] auto get_compute_command_pool() const noexcept -> vk::CommandPool { return m_compute_command_pool; }
        [[nodiscard]] auto get_transfer_command_pool() const noexcept -> vk::CommandPool { return m_transfer_command_pool; }

        [[nodiscard]] auto get_descriptor_allocator() noexcept -> descriptor_allocator& { return *m_descriptor_allocator; }
        [[nodiscard]] auto get_descriptor_layout_cache() noexcept -> descriptor_layout_cache& { return *m_descriptor_layout_cache; }
        [[nodiscard]] auto descriptor_factory_begin() -> descriptor_factory { return descriptor_factory{get_descriptor_layout_cache(), get_descriptor_allocator()}; }
        [[nodiscard]] auto get_window() const noexcept -> GLFWwindow* { return m_window; }
        [[nodiscard]] auto get_concurrent_frames() const noexcept -> std::uint32_t { return m_concurrent_frames; }
        [[nodiscard]] auto get_msaa_samples() const noexcept -> vk::SampleCountFlagBits { return m_msaa_samples; }
        [[nodiscard]] auto get_command_buffers() const noexcept -> eastl::span<const vk::CommandBuffer> { return m_command_buffers; }
        [[nodiscard]] auto get_current_frame() const noexcept -> std::uint32_t { return m_current_frame; }
        [[nodiscard]] auto get_image_index() const noexcept -> std::uint32_t { return m_image_index; }
        [[nodiscard]] auto get_pipeline_cache() const noexcept -> vk::PipelineCache { return m_pipeline_cache; }
        [[nodiscard]] auto get_scene_render_pass() const noexcept -> vk::RenderPass { return m_scene_render_pass; }
        [[nodiscard]] auto get_skybox_render_pass() const noexcept -> vk::RenderPass { return m_skybox_render_pass; }
        [[nodiscard]] auto get_ui_render_pass() const noexcept -> vk::RenderPass { return m_ui_render_pass; }
        [[nodiscard]] auto get_framebuffers() const noexcept -> eastl::span<const vk::Framebuffer> { return m_framebuffers; }
        [[nodiscard]] auto get_deferred_deletion_queue() noexcept -> deletion_queue& { return m_shutdown_deletion_queue; }

        [[nodiscard]] HOTPROC auto begin_frame(
            const DirectX::XMFLOAT4A& clear_color,
            vk::CommandBufferInheritanceInfo* out_inheritance_info = nullptr
        ) -> eastl::optional<command_buffer>;

        HOTPROC auto end_frame(command_buffer& cmd) -> void;
        auto begin_render_pass(command_buffer& cmd, vk::RenderPass pass, vk::SubpassContents contents) -> void;
        auto end_render_pass(command_buffer& cmd) -> void;
        auto on_resize() -> void;

        static auto create(const context_desc& desc) -> void;
        static auto shutdown() -> void;

    private:
        inline static constinit context* s_instance {};

        friend auto ctx() noexcept -> context&;

        auto boot_vulkan_core() -> void;
        auto create_sync_prims() -> void;
        auto create_command_pools() -> void;
        auto create_command_buffers() -> void;
        auto setup_depth_stencil() -> void;
        auto setup_render_pass() -> void;
        auto setup_frame_buffer() -> void;
        auto create_pipeline_cache() -> void;
        auto recreate_swapchain() -> void;
        auto create_msaa_target() -> void;

        auto destroy_depth_stencil() const -> void;
        auto destroy_msaa_target() const -> void;
        auto destroy_frame_buffer() const -> void;
        auto destroy_command_buffers() const -> void;
        auto destroy_sync_prims() const -> void;

        GLFWwindow* m_window = nullptr;
        std::uint32_t m_concurrent_frames = 3;
        vk::SampleCountFlagBits m_msaa_samples = vk::SampleCountFlagBits::e4;
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        deletion_queue m_shutdown_deletion_queue {};

        eastl::optional<device> m_device;
        eastl::optional<swapchain> m_swapchain;
        eastl::optional<descriptor_allocator> m_descriptor_allocator {};
        eastl::optional<descriptor_layout_cache> m_descriptor_layout_cache {};

        struct {
            eastl::fixed_vector<vk::Semaphore, 4> present_complete {}; // Swap chain image presentation
            eastl::fixed_vector<vk::Semaphore, 4> render_complete {}; // Command buffer submission and execution
        } m_semaphores {}; // Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering

        vk::CommandPool m_graphics_command_pool {};
        vk::CommandPool m_compute_command_pool {};
        vk::CommandPool m_transfer_command_pool {};

        eastl::fixed_vector<vk::CommandBuffer, 4> m_command_buffers {}; // Command buffers used for rendering
        eastl::fixed_vector<vk::Fence, 4> m_wait_fences {}; // Wait fences to sync command buffer access
        struct {
            vk::Image image {};
            vk::ImageView view {};
            VmaAllocation memory {};
        } m_depth_stencil {};

        vk::RenderPass m_scene_render_pass {};
        vk::RenderPass m_skybox_render_pass {};
        vk::RenderPass m_ui_render_pass {};
        eastl::fixed_vector<vk::Framebuffer, 4> m_framebuffers {};
        vk::PipelineCache m_pipeline_cache {};
        std::uint32_t m_current_frame = 0; // To select the correct sync objects, we need to keep track of the current frame
        std::uint32_t m_image_index = 0; // The current swap chain image index

        struct {
            struct {
                vk::Image image {};
                vk::ImageView view {};
                VmaAllocation memory {};
            } color {};
            struct {
                vk::Image image {};
                vk::ImageView view {};
                VmaAllocation memory {};
            } depth {};
        } m_msaa_target {};
        eastl::array<vk::ClearValue, 3> m_clear_values {};
    };

    // Get global vulkan context wrapper class
    [[nodiscard]] inline auto ctx() noexcept -> context& {
        passert(context::s_instance != nullptr);
        return *context::s_instance;
    }

    // Get global vulkan device wrapper class
    [[nodiscard]] inline auto dvc() noexcept -> const device& { return ctx().get_device(); }

    // Get global raw vulkan logical device
    [[nodiscard]] inline auto vkdvc() noexcept -> vk::Device { return dvc().get_logical_device(); }
}
