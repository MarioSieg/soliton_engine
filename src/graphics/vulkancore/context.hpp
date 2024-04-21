// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "device.hpp"
#include "swapchain.hpp"
#include "../imgui/imgui.h"
#include <DirectXMath.h>

namespace vkb {
    template <const vk::QueueFlagBits QueueType>
    concept is_queue_type = requires {
        requires
            QueueType == vk::QueueFlagBits::eGraphics
            || QueueType == vk::QueueFlagBits::eCompute
            || QueueType == vk::QueueFlagBits::eTransfer;
    };

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
        [[nodiscard]] auto get_graphics_command_pool() const noexcept -> vk::CommandPool { return m_graphics_command_pool; }
        [[nodiscard]] auto get_compute_command_pool() const noexcept -> vk::CommandPool { return m_compute_command_pool; }
        [[nodiscard]] auto get_transfer_command_pool() const noexcept -> vk::CommandPool { return m_transfer_command_pool; }
        template <const vk::QueueFlagBits QueueType> requires is_queue_type<QueueType>
        [[nodiscard]] auto start_command_buffer() const -> vk::CommandBuffer {
            vk::CommandBuffer cmd {};
            vk::CommandBufferAllocateInfo cmd_alloc_info {};
            vk::CommandPool pool {};
            if constexpr (QueueType == vk::QueueFlagBits::eGraphics) {
                pool = get_graphics_command_pool();
            } else if constexpr (QueueType == vk::QueueFlagBits::eCompute) {
                pool = get_compute_command_pool();
            } else if constexpr (QueueType == vk::QueueFlagBits::eTransfer) {
                pool = get_transfer_command_pool();
            } else {
                panic("Invalid queue type");
            }
            cmd_alloc_info.commandPool = pool;
            cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
            cmd_alloc_info.commandBufferCount = 1;
            vkcheck(m_device->get_logical_device().allocateCommandBuffers(&cmd_alloc_info, &cmd)); // TODO: not thread safe

            vk::CommandBufferBeginInfo cmd_begin_info {};
            cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            vkcheck(cmd.begin(&cmd_begin_info));
            return cmd;
        }
        template <const vk::QueueFlagBits QueueType, const bool Owned = true> requires is_queue_type<QueueType>
        auto flush_command_buffer(const vk::CommandBuffer cmd) const -> void {
            const vk::Device device = m_device->get_logical_device();
            if constexpr (Owned) {
                vkcheck(cmd.end());
            }
            constexpr vk::FenceCreateInfo fence_info {};
            vk::Fence fence {};
            vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));
            vk::SubmitInfo submit_info {};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;
            vk::Queue queue {};
            if constexpr (QueueType == vk::QueueFlagBits::eGraphics) {
                queue = m_device->get_graphics_queue();
            } else if constexpr (QueueType == vk::QueueFlagBits::eCompute) {
                queue = m_device->get_compute_queue();
            } else if constexpr (QueueType == vk::QueueFlagBits::eTransfer) {
                queue = m_device->get_transfer_queue();
            } else {
                panic("Invalid queue type");
            }
            vkcheck(queue.submit(1, &submit_info, fence));// TODO: not thread safe, use transfer queue
            vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));
            device.destroyFence(fence, &vkb::s_allocator);
            if constexpr (Owned) {
                vk::CommandPool pool {};
                if constexpr (QueueType == vk::QueueFlagBits::eGraphics) {
                    pool = get_graphics_command_pool();
                } else if constexpr (QueueType == vk::QueueFlagBits::eCompute) {
                    pool = get_compute_command_pool();
                } else if constexpr (QueueType == vk::QueueFlagBits::eTransfer) {
                    pool = get_transfer_command_pool();
                } else {
                    panic("Invalid queue type");
                }
                device.freeCommandBuffers(pool, 1, &cmd);
            }
        }
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

        HOTPROC auto begin_frame(const DirectX::XMFLOAT4A& clear_color, vk::CommandBufferInheritanceInfo* out_inheritance_info = nullptr) -> vk::CommandBuffer;
        HOTPROC auto end_frame(vk::CommandBuffer cmd_buf) -> void;
        auto render_imgui(ImDrawData* data, vk::CommandBuffer cmd_buf) -> void;
        auto on_resize() -> void;

    private:
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
        auto create_imgui_renderer() -> void;

        auto destroy_depth_stencil() const -> void;
        auto destroy_msaa_target() const -> void;
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
        vk::CommandPool m_graphics_command_pool {};
        vk::CommandPool m_compute_command_pool {};
        vk::CommandPool m_transfer_command_pool {};
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
    };

    // Get global vulkan context wrapper class
    [[nodiscard]] inline auto ctx() noexcept -> context& { return *context::s_instance; }

    // Get global vulkan device wrapper class
    [[nodiscard]] inline auto dvc() noexcept -> const device& { return ctx().get_device(); }

    // Get global raw vulkan logical device
    [[nodiscard]] inline auto vkdvc() noexcept -> vk::Device { return dvc().get_logical_device(); }
}
