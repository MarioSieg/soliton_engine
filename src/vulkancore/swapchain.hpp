// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "vma.hpp"
#include <GLFW/glfw3.h>

namespace vkb {
    class swapchain final : public no_copy, public no_move {
    public:
        swapchain(
            vk::Instance instance,
            vk::PhysicalDevice physical_device,
            vk::Device logical_device
        );
        ~swapchain();

        auto init_surface(GLFWwindow* window) -> void;

    private:
        struct buffer final {
            vk::Image image {};
            vk::ImageView view {};
        };

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
