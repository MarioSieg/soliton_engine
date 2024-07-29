// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../assetmgr/assetmgr.hpp"
#include "vulkancore/buffer.hpp"

#include <bx/allocator.h>

namespace lu::graphics {
    class texture_allocator final : public bx::AllocatorI {
    public:
        auto realloc(void* p, size_t size, size_t align, const char* filePath, std::uint32_t line) -> void* override;
    };
    extern constinit texture_allocator s_texture_allocator;

    class texture final : public assetmgr::asset {
    public:
        explicit texture(eastl::string&& asset_path);
        explicit texture(std::span<const std::byte> raw_mem);
        texture(
            vk::ImageType type,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t depth,
            std::uint32_t mip_levels,
            std::uint32_t array_size,
            vk::Format format,
            VmaMemoryUsage memory_usage,
            vk::ImageUsageFlags usage,
            vk::SampleCountFlagBits sample_count,
            vk::ImageLayout initial_layout,
            const void* data,
            std::size_t size,
            vk::ImageCreateFlags flags = {},
            vk::ImageTiling tiling = vk::ImageTiling::eOptimal
        );
        ~texture() override;

        [[nodiscard]] auto get_image() const noexcept -> const vk::Image& { return m_image; }
        [[nodiscard]] auto get_view() const noexcept -> const vk::ImageView& { return m_image_view; }
        [[nodiscard]] auto get_memory() const noexcept -> vk::DeviceMemory { return m_memory; }
        [[nodiscard]] auto get_allocation() const noexcept -> VmaAllocation { return m_allocation; }
        [[nodiscard]] auto get_allocator() const noexcept -> VmaAllocator { return m_allocator; }
        [[nodiscard]] auto get_mapped() const noexcept -> void* { return m_mapped; }
        [[nodiscard]] auto get_width() const noexcept -> std::uint32_t { return m_width; }
        [[nodiscard]] auto get_height() const noexcept -> std::uint32_t { return m_height; }
        [[nodiscard]] auto get_depth() const noexcept -> std::uint32_t { return m_depth; }
        [[nodiscard]] auto get_mip_levels() const noexcept -> std::uint32_t { return m_mip_levels; }
        [[nodiscard]] auto get_array_size() const noexcept -> std::uint32_t { return m_array_size; }
        [[nodiscard]] auto get_format() const noexcept -> vk::Format { return m_format; }
        [[nodiscard]] auto get_usage() const noexcept -> vk::ImageUsageFlags { return m_usage; }
        [[nodiscard]] auto get_memory_usage() const noexcept -> VmaMemoryUsage { return m_memory_usage; }
        [[nodiscard]] auto get_sample_count() const noexcept -> vk::SampleCountFlagBits { return m_sample_count; }
        [[nodiscard]] auto get_type() const noexcept -> vk::ImageType { return m_type; }
        [[nodiscard]] auto get_flags() const noexcept -> vk::ImageCreateFlags { return m_flags; }
        [[nodiscard]] auto get_tiling() const noexcept -> vk::ImageTiling { return m_tiling; }
        [[nodiscard]] auto get_sampler() const noexcept -> const vk::Sampler& { return m_sampler; }

        static auto set_image_layout_barrier(
            vk::CommandBuffer cmd_buf,
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::ImageSubresourceRange range,
            vk::PipelineStageFlags src_stage = vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlags dst_stage = vk::PipelineStageFlagBits::eAllCommands
        ) -> void;

    private:
        auto create(
            void* img,
            vk::ImageType type,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t depth,
            std::uint32_t mip_levels,
            std::uint32_t array_size,
            vk::Format format,
            VmaMemoryUsage memory_usage,
            vk::ImageUsageFlags usage,
            vk::SampleCountFlagBits sample_count,
            vk::ImageLayout initial_layout,
            const void* data,
            std::size_t size,
            vk::ImageCreateFlags flags = {},
            vk::ImageTiling tiling = vk::ImageTiling::eOptimal
        ) -> void;

        auto parse_from_raw_memory(std::span<const std::byte> texels) -> void;

        auto upload(
            void* img,
            std::size_t array_idx,
            std::size_t mip_level,
            const void* data,
            std::size_t size,
            vk::ImageLayout src_layout = vk::ImageLayout::eUndefined,
            vk::ImageLayout dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal
        ) const -> void;

        auto generate_mips(
            bool transfer_only,
            vk::ImageLayout src_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor,
            vk::Filter filter = vk::Filter::eLinear
        ) const -> void;

        auto create_sampler() -> void;

        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
        std::uint32_t m_depth = 0;
        std::uint32_t m_mip_levels = 0;
        std::uint32_t m_array_size = 0;
        vk::Format m_format = vk::Format::eUndefined;
        vk::ImageUsageFlags m_usage {};
        vk::SampleCountFlagBits m_sample_count = vk::SampleCountFlagBits::e1;
        vk::ImageType m_type = vk::ImageType::e2D;
        vk::ImageCreateFlags m_flags {};
        vk::ImageTiling m_tiling = vk::ImageTiling::eOptimal;
        vk::Image m_image {};
        vk::ImageView m_image_view {};
        vk::DeviceMemory m_memory {};
        VmaMemoryUsage m_memory_usage = VMA_MEMORY_USAGE_UNKNOWN;
        VmaAllocation m_allocation {};
        VmaAllocator m_allocator {};
        void* m_mapped = nullptr;
        vk::Sampler m_sampler {};
    };
}
