// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"

#include "../assetmgr/assetmgr.hpp"
#include "utils/texture_loader.hpp"
#include "vulkancore/context.hpp"
#include "vulkancore/buffer.hpp"
#include "vulkancore/command_buffer.hpp"

namespace lu::graphics {
    texture::texture(eastl::string&& asset_path) : asset {assetmgr::asset_source::filesystem, std::move(asset_path)} {
        log_info("Loading texture '{}'", get_asset_path());
        eastl::vector<std::byte> texels {};
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor &acc) {
            if (!acc.load_bin_file(get_asset_path().c_str(), texels)) {
                panic("Failed to load texture '{}'", get_asset_path());
            }
        });
        parse_from_raw_memory(texels);
    }

    texture::texture(const eastl::span<const std::byte> file_mem) : asset {assetmgr::asset_source::memory} {
        parse_from_raw_memory(file_mem);
    }

    texture::texture(const texture_descriptor& desc, const eastl::optional<texture_data_supplier>& data) : asset {assetmgr::asset_source::memory} {
        create(desc, data);
    }

    texture::~texture() {
        if (m_image) {
            vkb::vkdvc().destroySampler(m_sampler, vkb::get_alloc());
            vkb::vkdvc().destroyImageView(m_image_view, vkb::get_alloc());
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = nullptr;
        }
    }

    auto texture::create(const texture_descriptor& desc, const eastl::optional<texture_data_supplier>& data) -> void {
        m_desc = desc;
        m_approx_byte_size = sizeof(*this);
        m_allocator = vkb::dvc().get_allocator();

        using enum texture_descriptor::mipmap_creation_mode;

        switch (desc.mipmap_mode) {
            case present_in_data: passert(data.has_value()); break;
            case generate:
                if (m_desc.miplevel_count <= 1)
                    m_desc.miplevel_count = get_mip_count(m_desc.width, m_desc.height);
            break;
            default:;
        }

        vk::ImageCreateInfo image_info {};
        image_info.imageType = m_desc.type;
        image_info.extent.width = m_desc.width;
        image_info.extent.height = m_desc.height;
        image_info.extent.depth = m_desc.depth;
        image_info.mipLevels = m_desc.miplevel_count;
        image_info.arrayLayers = m_desc.array_size;
        image_info.format = m_desc.format;
        image_info.tiling = m_desc.tiling;
        image_info.initialLayout = m_desc.initial_layout;
        image_info.usage = m_desc.usage;
        image_info.samples = m_desc.sample_count;
        image_info.sharingMode = vk::SharingMode::eExclusive;
        image_info.flags = m_desc.flags;

        VmaAllocationInfo alloc_info {};
        VmaAllocationCreateInfo alloc_create_info {};

        alloc_create_info.usage = m_desc.memory_usage;
        alloc_create_info.flags = m_desc.memory_usage == VMA_MEMORY_USAGE_CPU_ONLY
                || m_desc.memory_usage == VMA_MEMORY_USAGE_GPU_TO_CPU ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
        vkccheck(vmaCreateImage(
            m_allocator,
            reinterpret_cast<VkImageCreateInfo*>(&image_info),
            &alloc_create_info,
            reinterpret_cast<VkImage*>(&m_image),
            &m_allocation,
            &alloc_info
        ));

        m_memory = alloc_info.deviceMemory;
        m_mapped = alloc_info.pMappedData;

        if (data.has_value()) {
            passert(data->data != nullptr && data->size != 0);
            m_buf_size = data->size;
            m_approx_byte_size += m_buf_size;
            vkb::command_buffer copy_cmd {vk::QueueFlagBits::eTransfer};
            copy_cmd.begin();

            vk::ImageSubresourceRange subresource_range {};
            subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource_range.baseMipLevel = 0;
            subresource_range.baseArrayLayer = 0;
            subresource_range.levelCount = m_desc.miplevel_count;
            subresource_range.layerCount = m_desc.array_size;

            // Image barrier for optimal image (target)
            // Optimal image will be used as destination for the copy
            copy_cmd.set_image_layout_barrier(
                m_image,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );

            copy_cmd.end();
            copy_cmd.flush();

            upload(0, 0, data->data, data->size, data->mip_copy_regions, vk::ImageLayout::eTransferDstOptimal);

            if (m_desc.mipmap_mode != no_mips && m_desc.miplevel_count > 1) {
                const bool transfer_only = m_desc.mipmap_mode == present_in_data; // if mips were loaded from file (! generated), we just copy them
                generate_mips(transfer_only);
            }
        }

        // image view:
        vk::ImageViewCreateInfo image_view_ci {};
        image_view_ci.viewType = (m_desc.flags & vk::ImageCreateFlagBits::eCubeCompatible) ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;
        image_view_ci.format = m_desc.format;
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_ci.subresourceRange.baseMipLevel = 0;
        image_view_ci.subresourceRange.baseArrayLayer = 0;
        image_view_ci.subresourceRange.layerCount = m_desc.array_size;
        image_view_ci.subresourceRange.levelCount = m_desc.miplevel_count;
        image_view_ci.image = m_image;
        vkcheck(vkb::vkdvc().createImageView(&image_view_ci, vkb::get_alloc(), &m_image_view));

        create_sampler(
            m_desc.sampler.min_filter,
            m_desc.sampler.mag_filter,
            m_desc.sampler.mipmap_mode,
            m_desc.sampler.address_mode,
            m_desc.sampler.enable_anisotropy
        );
    }

    auto texture::parse_from_raw_memory(const eastl::span<const std::byte> buf) -> void {
        const bool result = raw_parse_texture(buf, [&](const texture_descriptor& info, const texture_data_supplier& data) {
            create(info, data);
        });
        if (!result) [[unlikely]] {
            panic("Failed to parse texture from raw memory");
        }
    }

    auto texture::upload(
        const std::size_t array_idx,
        const std::size_t mip_level,
        const void* const data,
        const std::size_t size,
        const eastl::span<const vk::BufferImageCopy> regions,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout
    ) const -> void {
        vkb::buffer staging {
            size,
            0,
            vk::BufferUsageFlagBits::eTransferSrc,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            data
        };

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = mip_level;
        subresource_range.levelCount = m_desc.miplevel_count;
        subresource_range.layerCount = 1;
        subresource_range.baseArrayLayer = array_idx;

        vkb::command_buffer copy_cmd {vk::QueueFlagBits::eTransfer};
        copy_cmd.begin();

        if (src_layout != vk::ImageLayout::eTransferDstOptimal) {
            copy_cmd.set_image_layout_barrier(
                m_image,
                src_layout,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );
        }

        copy_cmd.copy_buffer_to_image(
            staging.get_buffer(),
            m_image,
            regions
        );

        if (dst_layout != vk::ImageLayout::eTransferDstOptimal) {
            copy_cmd.set_image_layout_barrier(
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                dst_layout,
                subresource_range
            );
        }

        copy_cmd.end();
        copy_cmd.flush();
    }

    auto texture::generate_mips(
        const bool transfer_only,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout,
        const vk::ImageAspectFlags aspect_mask,
        const vk::Filter filter
    ) const -> void {
        vkb::command_buffer blit_cmd {vk::QueueFlagBits::eGraphics};
        blit_cmd.begin();

        vk::ImageSubresourceRange intial_subresource_range {};
        intial_subresource_range.aspectMask = aspect_mask;
        intial_subresource_range.baseMipLevel = 1;
        intial_subresource_range.levelCount = m_desc.miplevel_count - 1;
        intial_subresource_range.layerCount = m_desc.array_size;
        intial_subresource_range.baseArrayLayer = 0;

        blit_cmd.set_image_layout_barrier(
            m_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            intial_subresource_range
        );

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = aspect_mask;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        for (std::uint32_t i = 0; i < m_desc.array_size; ++i) {
            std::uint32_t mip_w = m_desc.width;
            std::uint32_t mip_h = m_desc.height;
            for (std::uint32_t j = 1; j < m_desc.miplevel_count; ++j) {
                subresource_range.baseMipLevel = j - 1;
                subresource_range.baseArrayLayer = i;
                vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal;
                if (j == 1) {
                    layout = src_layout;
                }
                if (layout != vk::ImageLayout::eTransferSrcOptimal) {
                    blit_cmd.set_image_layout_barrier(
                        m_image,
                        layout,
                        vk::ImageLayout::eTransferSrcOptimal,
                        subresource_range
                    );
                }
                if (!transfer_only) { // If we're only doing a transfer, we don't need to blit. TODO: refractor
                    vk::ImageBlit blit {};
                    blit.srcOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.srcOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w), static_cast<std::int32_t>(mip_h), 1 };
                    blit.srcSubresource.aspectMask = aspect_mask;
                    blit.srcSubresource.mipLevel = j - 1;
                    blit.srcSubresource.baseArrayLayer = i;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.dstOffsets[1] = vk::Offset3D {
                        static_cast<std::int32_t>(mip_w > 1 ? mip_w >> 1 : 1),
                        static_cast<std::int32_t>(mip_h > 1 ? mip_h >> 1 : 1),
                        1
                    };
                    blit.dstSubresource.aspectMask = aspect_mask;
                    blit.dstSubresource.mipLevel = j;
                    blit.dstSubresource.baseArrayLayer = i;
                    blit.dstSubresource.layerCount = 1;
                    (*blit_cmd).blitImage(
                        m_image,
                        vk::ImageLayout::eTransferSrcOptimal,
                        m_image,
                        vk::ImageLayout::eTransferDstOptimal,
                        1,
                        &blit,
                        filter
                    );
                }
                blit_cmd.set_image_layout_barrier(
                    m_image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    dst_layout,
                    subresource_range
                );
                mip_w >>= !!(mip_w > 1);
                mip_h >>= !!(mip_h > 1);
            }
        }

        subresource_range.baseMipLevel = m_desc.miplevel_count - 1;
        blit_cmd.set_image_layout_barrier(
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            dst_layout,
            subresource_range
        );

        blit_cmd.end();
        blit_cmd.flush();
        log_info("{} mip-chain with {} maps", transfer_only ? "Loaded" : "Generated", m_desc.miplevel_count);
    }

    auto texture::create_sampler(
        const vk::Filter mag_filter,
        const vk::Filter min_filter,
        const vk::SamplerMipmapMode mipmap_mode,
        const vk::SamplerAddressMode address_mode,
        const bool aniso_enable
    ) -> void {
        const bool supports_anisotropy = vkb::dvc().get_physical_device_features().samplerAnisotropy;

        vk::SamplerCreateInfo sampler_info {};
        sampler_info.magFilter = mag_filter;
        sampler_info.minFilter = min_filter;
        sampler_info.mipmapMode = mipmap_mode;
        sampler_info.addressModeU = address_mode;
        sampler_info.addressModeV = address_mode;
        sampler_info.addressModeW = address_mode;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.compareOp = vk::CompareOp::eAlways;
        sampler_info.compareEnable = vk::False;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = static_cast<float>(m_desc.miplevel_count);
        sampler_info.maxAnisotropy = aniso_enable && supports_anisotropy ? vkb::dvc().get_physical_device_props().limits.maxSamplerAnisotropy : 1.0f;
        sampler_info.anisotropyEnable = aniso_enable && supports_anisotropy ? vk::True : vk::False;
        sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        sampler_info.unnormalizedCoordinates = vk::False;
        vkcheck(vkb::vkdvc().createSampler(&sampler_info, vkb::get_alloc(), &m_sampler));
    }
}
