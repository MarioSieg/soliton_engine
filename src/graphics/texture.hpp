// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../assetmgr/assetmgr.hpp"
#include "vulkancore/buffer.hpp"

namespace lu::graphics {
    struct texture_descriptor final {
        enum class mipmap_creation_mode {
            no_mips,
            present_in_data,
            generate
        };

        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t depth = 1;
        std::uint32_t miplevel_count = 1;
        mipmap_creation_mode mipmap_mode = mipmap_creation_mode::no_mips;
        std::uint32_t array_size = 1;
        vk::Format format = vk::Format::eUndefined;
        VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
        vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
        vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
        vk::ImageType type = vk::ImageType::e2D;
        vk::ImageCreateFlags flags = vk::ImageCreateFlags {};
        bool is_cubemap = false;
        struct {
            vk::Filter mag_filter = vk::Filter::eLinear;
            vk::Filter min_filter = vk::Filter::eLinear;
            vk::SamplerMipmapMode mipmap_mode = vk::SamplerMipmapMode::eLinear;
            vk::SamplerAddressMode address_mode = vk::SamplerAddressMode::eRepeat;
            bool enable_anisotropy = true;
        } sampler {};
    };

    struct texture_data_supplier final {
        eastl::vector<vk::BufferImageCopy> mip_copy_regions {};
        const void* data = nullptr;
        std::size_t size = 0;
    };

    class texture final : public assetmgr::asset {
    public:
        static constexpr auto get_mip_count(const std::uint32_t w, const std::uint32_t h) noexcept -> std::uint32_t {
            return static_cast<std::uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1; // [log2(max(w, h))] + 1
        }

        explicit texture(eastl::string&& asset_path); // Load from file
        explicit texture(eastl::span<const std::byte> file_mem); // Parse file from memory
        explicit texture(const texture_descriptor& desc, const eastl::optional<texture_data_supplier>& data); // Create manually
        ~texture() override;

        [[nodiscard]] auto image() const noexcept -> const vk::Image& { return m_image; }
        [[nodiscard]] auto image_view() const noexcept -> const vk::ImageView& { return m_image_view; }
        [[nodiscard]] auto desc() const noexcept -> const texture_descriptor& { return m_desc; }
        [[nodiscard]] auto memory() const noexcept -> vk::DeviceMemory { return m_memory; }
        [[nodiscard]] auto allocation() const noexcept -> VmaAllocation { return m_allocation; }
        [[nodiscard]] auto allocator() const noexcept -> VmaAllocator { return m_allocator; }
        [[nodiscard]] auto mapped() const noexcept -> void* { return m_mapped; }
        [[nodiscard]] auto width() const noexcept -> std::uint32_t { return m_desc.width; }
        [[nodiscard]] auto height() const noexcept -> std::uint32_t { return m_desc.height; }
        [[nodiscard]] auto depth() const noexcept -> std::uint32_t { return m_desc.depth; }
        [[nodiscard]] auto mip_levels() const noexcept -> std::uint32_t { return m_desc.miplevel_count; }
        [[nodiscard]] auto array_size() const noexcept -> std::uint32_t { return m_desc.array_size; }
        [[nodiscard]] auto is_cubemap() const noexcept -> bool { return m_desc.is_cubemap; }
        [[nodiscard]] auto byte_size() const noexcept -> std::size_t { return m_buf_size; }
        [[nodiscard]] auto format() const noexcept -> vk::Format { return m_desc.format; }
        [[nodiscard]] auto usage_flags() const noexcept -> vk::ImageUsageFlags { return m_desc.usage; }
        [[nodiscard]] auto memory_usage() const noexcept -> VmaMemoryUsage { return m_desc.memory_usage; }
        [[nodiscard]] auto sample_count() const noexcept -> vk::SampleCountFlagBits { return m_desc.sample_count; }
        [[nodiscard]] auto type() const noexcept -> vk::ImageType { return m_desc.type; }
        [[nodiscard]] auto flags() const noexcept -> vk::ImageCreateFlags { return m_desc.flags; }
        [[nodiscard]] auto tiling() const noexcept -> vk::ImageTiling { return m_desc.tiling; }
        [[nodiscard]] auto sampler() const noexcept -> const vk::Sampler& { return m_sampler; }

    private:
        auto create(const texture_descriptor& desc, const eastl::optional<texture_data_supplier>& data) -> void;
        auto parse_from_raw_memory(eastl::span<const std::byte> buf) -> void;

        auto upload(
            std::size_t array_idx,
            std::size_t mip_level,
            const void* data,
            std::size_t size,
            eastl::span<const vk::BufferImageCopy> regions,
            vk::ImageLayout src_layout = vk::ImageLayout::eUndefined,
            vk::ImageLayout dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal
        ) const -> void;

        auto generate_mips(
            vk::ImageLayout src_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor,
            vk::Filter filter = vk::Filter::eLinear
        ) const -> void;

        auto create_sampler(
            vk::Filter mag_filter,
            vk::Filter min_filter,
            vk::SamplerMipmapMode mipmap_mode,
            vk::SamplerAddressMode address_mode,
            bool aniso_enable
        ) -> void;

        texture_descriptor m_desc {};
        std::size_t m_buf_size {};

        vk::Image m_image {};
        vk::ImageView m_image_view {};
        vk::DeviceMemory m_memory {};
        VmaAllocation m_allocation {};
        VmaAllocator m_allocator {};
        void* m_mapped = nullptr;
        vk::Sampler m_sampler {};
    };
}
