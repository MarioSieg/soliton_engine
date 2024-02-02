// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"
#include "vma.hpp"

// TODO: RAII

namespace vkb {
    class buffer final : public no_copy, public no_move {
    public:
        auto create(
            std::size_t size,
            std::size_t alignment,
            vk::BufferUsageFlags buffer_usage,
            VmaMemoryUsage memory_usage,
            VmaAllocationCreateFlags create_flags = 0,
            const void* data = nullptr
        ) -> void;
        ~buffer();
        [[nodiscard]] auto get_size() const noexcept -> std::size_t { return m_size; }
        [[nodiscard]] auto get_mapped_ptr() const noexcept -> void* { return m_mapped; }
        [[nodiscard]] auto get_buffer() const noexcept -> const vk::Buffer& { return m_buffer; }
        [[nodiscard]] auto get_memory() const noexcept -> vk::DeviceMemory { return m_memory; }
        [[nodiscard]] auto get_device_address() const noexcept -> vk::DeviceAddress { return m_device_address; }
        [[nodiscard]] auto get_allocator() const noexcept -> VmaAllocator { return m_allocator; }
        [[nodiscard]] auto get_allocation() const noexcept -> VmaAllocation { return m_allocation; }
        [[nodiscard]] auto get_memory_usage() const noexcept -> VmaMemoryUsage { return m_memory_usage; }
        [[nodiscard]] auto get_memory_properties() const noexcept -> vk::MemoryPropertyFlags { return m_memory_properties; }
        [[nodiscard]] auto get_usage() const noexcept -> vk::BufferUsageFlags { return m_usage; }

        auto upload_data(const void* data, std::size_t size, std::size_t offset) -> void;

    private:
        std::size_t m_size {};
        void* m_mapped {};
        vk::Buffer m_buffer {};
        vk::DeviceMemory m_memory {};
        vk::DeviceAddress m_device_address {};
        VmaAllocator m_allocator {};
        VmaAllocation m_allocation {};
        VmaMemoryUsage m_memory_usage {};
        vk::MemoryPropertyFlags m_memory_properties {};
        vk::BufferUsageFlags m_usage {};
    };
}
