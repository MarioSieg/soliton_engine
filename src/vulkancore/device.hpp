// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "vma.hpp"

namespace vkb {
    class device final : public no_copy, public no_move {
    public:
        static constexpr std::uint32_t k_vulkan_api_version = VK_API_VERSION_1_2;

        explicit device(
            bool enable_validation,
            bool require_stencil = false
        );
        ~device();

        operator vk::Device() const noexcept { return m_logical_device; }

        [[nodiscard]] auto get_instance() const noexcept -> vk::Instance { return m_instance; }
        [[nodiscard]] auto get_physical_device() const noexcept -> vk::PhysicalDevice { return m_physical_device; }
        [[nodiscard]] auto get_logical_device() const noexcept -> vk::Device { return m_logical_device; }
        [[nodiscard]] auto get_physical_device_props() const noexcept -> const vk::PhysicalDeviceProperties& { return m_device_properties; }
        [[nodiscard]] auto get_physical_device_features() const noexcept -> const vk::PhysicalDeviceFeatures& { return m_device_features; }
        [[nodiscard]] auto get_physical_device_enabled_features() const noexcept -> const vk::PhysicalDeviceFeatures& { return m_enabled_features; }
        [[nodiscard]] auto get_supported_device_extensions() const noexcept -> std::span<const vk::ExtensionProperties> { return m_supported_device_extensions; }
        [[nodiscard]] auto get_supported_instance_extensions() const noexcept -> std::span<const std::string> { return m_supported_instance_extensions; }
        [[nodiscard]] auto get_depth_format() const noexcept -> vk::Format { return m_depth_format; }
        [[nodiscard]] auto get_mem_type_idx(std::uint32_t type_bits, vk::MemoryPropertyFlags properties, vk::Bool32& found) const -> std::uint32_t;
        [[nodiscard]] auto get_mem_type_idx_or_panic(std::uint32_t type_bits, vk::MemoryPropertyFlags properties) const -> std::uint32_t {
            vk::Bool32 found {};
            const auto idx = get_mem_type_idx(type_bits, properties, found);
            passert(found == vk::True);
            return idx;
        }
        [[nodiscard]] auto get_graphics_queue() const noexcept -> vk::Queue { return m_graphics_queue; }
        [[nodiscard]] auto get_compute_queue() const noexcept -> vk::Queue { return m_compute_queue; }
        [[nodiscard]] auto get_transfer_queue() const noexcept -> vk::Queue { return m_transfer_queue; }
        [[nodiscard]] auto get_graphics_queue_idx() const noexcept -> std::uint32_t { return m_queue_families.graphics; }
        [[nodiscard]] auto get_compute_queue_idx() const noexcept -> std::uint32_t { return m_queue_families.compute; }
        [[nodiscard]] auto get_transfer_queue_idx() const noexcept -> std::uint32_t { return m_queue_families.transfer; }
        [[nodiscard]] auto get_allocator() const noexcept -> VmaAllocator { return m_allocator; }

    private:
        static inline constinit std::array<const char*, 2> k_device_extensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME
        };
        static constexpr vk::PhysicalDeviceFeatures k_enabled_features = [] {
            vk::PhysicalDeviceFeatures enabled_features {};
            enabled_features.samplerAnisotropy = vk::True;
            return enabled_features;
        }();

        auto create_instance() -> void;
        auto find_physical_device() -> void;
        auto create_logical_device(
            const vk::PhysicalDeviceFeatures& enabled_features,
            std::span<const char*> enabled_extensions,
            void* next_chain,
            vk::QueueFlags requested_queue_types = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer
        ) -> void;
        auto init_vma() -> void;
        [[nodiscard]] auto get_queue_family_index(vk::QueueFlags flags) const noexcept -> std::uint32_t;
        [[nodiscard]] auto find_supported_depth_format(bool stencil_required, vk::Format& out_format) const -> bool;

        bool m_enable_validation = false;
        std::uint32_t m_api_version = 0;
        std::vector<std::string> m_supported_instance_extensions {};
        vk::Instance m_instance {};

        PFN_vkCreateDebugUtilsMessengerEXT m_create_debug_utils_messenger_ext {};
        PFN_vkDestroyDebugUtilsMessengerEXT m_destroy_debug_utils_messenger_ext {};
        vk::DebugUtilsMessengerCreateInfoEXT m_debug_utils_messenger_ci {};
        vk::DebugUtilsMessengerEXT m_debug_utils_messenger {};

        vk::PhysicalDevice m_physical_device {};
        vk::Device m_logical_device {};
        vk::PhysicalDeviceProperties m_device_properties {};
        vk::PhysicalDeviceFeatures m_device_features {};
        vk::PhysicalDeviceFeatures m_enabled_features {};
        std::vector<vk::ExtensionProperties> m_supported_device_extensions {};
        vk::PhysicalDeviceMemoryProperties m_memory_properties {};
        std::vector<vk::QueueFamilyProperties> m_queue_family_properties {};
        VmaAllocator m_allocator {};
        vk::Queue m_graphics_queue {};
        vk::Queue m_compute_queue {};
        vk::Queue m_transfer_queue {};
        vk::Format m_depth_format {};
        struct {
            std::uint32_t graphics = 0;
            std::uint32_t compute = 0;
            std::uint32_t transfer = 0;
        } m_queue_families {};
    };
}
