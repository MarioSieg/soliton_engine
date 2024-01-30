// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "vma.hpp"
#include <GLFW/glfw3.h>

namespace vkb {
    class swapchain final : public no_copy, public no_move {
    public:
        struct buffer final {
            vk::Image image {};
            vk::ImageView view {};
        };

        swapchain(
            vk::Instance instance,
            vk::PhysicalDevice physical_device,
            vk::Device logical_device
        );
        ~swapchain();

        auto init_surface(GLFWwindow* window) -> void;
        auto create(
            std::uint32_t& w,
            std::uint32_t& h,
            bool vsync,
            bool fullscreen
        ) -> void;
        [[nodiscard]] auto acquire_next_image(vk::Semaphore present_complete_semaphore, std::uint32_t& idx) const -> vk::Result;
        [[nodiscard]] auto queue_present(
            vk::Queue queue,
            std::uint32_t image_index,
            vk::Semaphore wait_semaphore = nullptr
        ) const -> vk::Result;
        [[nodiscard]] auto get_queue_node_index() const noexcept -> std::uint32_t { return m_queue_node_index; }
        [[nodiscard]] auto get_image_count() const noexcept -> std::uint32_t { return m_image_count; }
        [[nodiscard]] auto get_buffer(std::uint32_t index) const noexcept -> const buffer& { return m_buffers[index]; }
        [[nodiscard]] auto get_format() const noexcept -> vk::Format { return m_format; }
        [[nodiscard]] auto get_color_space() const noexcept -> vk::ColorSpaceKHR { return m_color_space; }
        [[nodiscard]] auto get_surface() const noexcept -> vk::SurfaceKHR { return m_surface; }
        [[nodiscard]] auto get_swapchain() const noexcept -> vk::SwapchainKHR { return m_swapchain; }
        [[nodiscard]] auto get_images() const noexcept -> std::span<const vk::Image> { return m_images; }

    private:
        vk::Instance m_instance {};
        vk::PhysicalDevice m_physical_device {};
        vk::Device m_logical_device {};
        vk::SurfaceKHR m_surface {};
        vk::Format m_format {};
        vk::ColorSpaceKHR m_color_space {};
        vk::SwapchainKHR m_swapchain {};
        std::uint32_t m_image_count {};
        std::vector<vk::Image> m_images {};
        std::vector<buffer> m_buffers {};
        std::uint32_t m_queue_node_index {};
    };
}
