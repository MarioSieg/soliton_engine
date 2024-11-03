// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "context.hpp"

namespace lu::vkb {
    class buffer : public no_copy, public no_move {
    public:
        buffer(
            std::size_t size,
            std::size_t alignment,
            vk::BufferUsageFlags buffer_usage,
            VmaMemoryUsage memory_usage,
            VmaAllocationCreateFlags create_flags = 0,
            const void* data = nullptr,
            bool manual_flush = false
        );
        virtual ~buffer();
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

        auto upload_data(const void* data, std::size_t size, std::size_t offset) const -> void;

    protected:
        std::size_t m_size {};
        mutable void* m_mapped {};
        vk::Buffer m_buffer {};
        vk::DeviceMemory m_memory {};
        vk::DeviceAddress m_device_address {};
        VmaAllocator m_allocator {};
        VmaAllocation m_allocation {};
        VmaMemoryUsage m_memory_usage {};
        vk::MemoryPropertyFlags m_memory_properties {};
        vk::BufferUsageFlags m_usage {};
    };

    template <typename T>
    concept is_uniform_buffer = requires {
        std::is_standard_layout_v<T>;
        sizeof(T) % (4 * sizeof(float)) == 0;
    };

    template <typename T> requires is_uniform_buffer<T>
    class uniform_buffer : public buffer {
    public:
        template <typename... Args>
        explicit uniform_buffer(
            const vk::BufferUsageFlags buffer_usage = vk::BufferUsageFlagBits::eUniformBuffer,
            const VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            const VmaAllocationCreateFlags create_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            Args&&... args
        ) : buffer {
            vkb::ctx().compute_aligned_dynamic_ubo_size(sizeof(T)) * vkb::ctx().get_concurrent_frame_count(),
            0,
            buffer_usage,
            memory_usage,
            create_flags,
            nullptr,
            true
        }, m_aligned_size{vkb::ctx().compute_aligned_dynamic_ubo_size(sizeof(T))} {
            m_aligned_tmp = static_cast<T*>(mi_malloc_aligned(m_aligned_size, m_aligned_size));
            new (m_aligned_tmp) T {eastl::forward<Args>(args)...};
        }
        virtual ~uniform_buffer() override {
            m_aligned_tmp->~T();
            mi_free(m_aligned_tmp);
        }

        auto set(const T& data) const noexcept -> void {
            *m_aligned_tmp = data;
            const auto offset = m_aligned_size * vkb::ctx().get_current_concurrent_frame_idx();
            panic_assert((std::bit_cast<std::uintptr_t>(m_mapped) + offset) % vkb::dvc().get_physical_device_props().limits.minUniformBufferOffsetAlignment == 0);
            std::memcpy(static_cast<std::byte*>(m_mapped) + offset, m_aligned_tmp, sizeof(T));
            vmaFlushAllocation(m_allocator, m_allocation, offset, sizeof(T)); // Manual flush
        }

        [[nodiscard]] inline auto get_dynamic_aligned_size() const noexcept -> std::size_t { return m_aligned_size; }

    private:
        T* m_aligned_tmp {};
        std::size_t m_aligned_size {};
    };
}
