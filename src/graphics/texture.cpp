// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "texture.hpp"

#include "../assetmgr/assetmgr.hpp"
#include "vulkancore/context.hpp"
#include "vulkancore/buffer.hpp"

#include <bimg/bimg.h>
#include <bimg/decode.h>

namespace graphics {
    static constexpr bimg::TextureFormat::Enum k_fallback_format = bimg::TextureFormat::RGBA8; // Textures are converted to this when native format is not supported

    struct texture_format_info {
        VkFormat fmt {};
        VkFormat fmt_srv {};
        VkFormat fmt_dsv {};
        VkFormat fmt_srgb {};
        VkComponentMapping mapping {};
    };

    static constexpr texture_format_info k_texture_format_map[] = {
#define $_ VK_COMPONENT_SWIZZLE_IDENTITY
#define $0 VK_COMPONENT_SWIZZLE_ZERO
#define $1 VK_COMPONENT_SWIZZLE_ONE
#define $R VK_COMPONENT_SWIZZLE_R
#define $G VK_COMPONENT_SWIZZLE_G
#define $B VK_COMPONENT_SWIZZLE_B
#define $A VK_COMPONENT_SWIZZLE_A
		{ VK_FORMAT_BC1_RGB_UNORM_BLOCK,       VK_FORMAT_BC1_RGB_UNORM_BLOCK,      VK_FORMAT_UNDEFINED,           VK_FORMAT_BC1_RGB_SRGB_BLOCK,       { $_, $_, $_, $_ } }, // BC1
		{ VK_FORMAT_BC2_UNORM_BLOCK,           VK_FORMAT_BC2_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC2_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC2
		{ VK_FORMAT_BC3_UNORM_BLOCK,           VK_FORMAT_BC3_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC3_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC3
		{ VK_FORMAT_BC4_UNORM_BLOCK,           VK_FORMAT_BC4_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC4
		{ VK_FORMAT_BC5_UNORM_BLOCK,           VK_FORMAT_BC5_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC5
		{ VK_FORMAT_BC6H_SFLOAT_BLOCK,         VK_FORMAT_BC6H_SFLOAT_BLOCK,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC6H
		{ VK_FORMAT_BC7_UNORM_BLOCK,           VK_FORMAT_BC7_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC7_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC7
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ETC1
		{ VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,   { $_, $_, $_, $_ } }, // ETC2
		{ VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A
		{ VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A1
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12A
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14A
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC22
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC24
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATC
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCE
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCI
		{ VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      VK_FORMAT_ASTC_4x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_4x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC4x4
		{ VK_FORMAT_ASTC_5x4_UNORM_BLOCK,      VK_FORMAT_ASTC_5x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x4
		{ VK_FORMAT_ASTC_5x5_UNORM_BLOCK,      VK_FORMAT_ASTC_5x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x5
		{ VK_FORMAT_ASTC_6x5_UNORM_BLOCK,      VK_FORMAT_ASTC_6x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x5
		{ VK_FORMAT_ASTC_6x6_UNORM_BLOCK,      VK_FORMAT_ASTC_6x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x6
		{ VK_FORMAT_ASTC_8x5_UNORM_BLOCK,      VK_FORMAT_ASTC_8x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x5
		{ VK_FORMAT_ASTC_8x6_UNORM_BLOCK,      VK_FORMAT_ASTC_8x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x6
		{ VK_FORMAT_ASTC_8x8_UNORM_BLOCK,      VK_FORMAT_ASTC_8x8_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x8_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x8
		{ VK_FORMAT_ASTC_10x5_UNORM_BLOCK,     VK_FORMAT_ASTC_10x5_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x5_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x5
		{ VK_FORMAT_ASTC_10x6_UNORM_BLOCK,     VK_FORMAT_ASTC_10x6_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x6_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x6
		{ VK_FORMAT_ASTC_10x8_UNORM_BLOCK,     VK_FORMAT_ASTC_10x8_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x8_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x8
		{ VK_FORMAT_ASTC_10x10_UNORM_BLOCK,    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC10x10
		{ VK_FORMAT_ASTC_12x10_UNORM_BLOCK,    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x10
		{ VK_FORMAT_ASTC_12x12_UNORM_BLOCK,    VK_FORMAT_ASTC_12x12_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x12_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x12
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // Unknown
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R1
		{ VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $0, $0, $0, $R } }, // A8
		{ VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_R8_SRGB,                  { $_, $_, $_, $_ } }, // R8
		{ VK_FORMAT_R8_SINT,                   VK_FORMAT_R8_SINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8I
		{ VK_FORMAT_R8_UINT,                   VK_FORMAT_R8_UINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8U
		{ VK_FORMAT_R8_SNORM,                  VK_FORMAT_R8_SNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8S
		{ VK_FORMAT_R16_UNORM,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16
		{ VK_FORMAT_R16_SINT,                  VK_FORMAT_R16_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16I
		{ VK_FORMAT_R16_UINT,                  VK_FORMAT_R16_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16U
		{ VK_FORMAT_R16_SFLOAT,                VK_FORMAT_R16_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16F
		{ VK_FORMAT_R16_SNORM,                 VK_FORMAT_R16_SNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16S
		{ VK_FORMAT_R32_SINT,                  VK_FORMAT_R32_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32I
		{ VK_FORMAT_R32_UINT,                  VK_FORMAT_R32_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32U
		{ VK_FORMAT_R32_SFLOAT,                VK_FORMAT_R32_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32F
		{ VK_FORMAT_R8G8_UNORM,                VK_FORMAT_R8G8_UNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8_SRGB,                { $_, $_, $_, $_ } }, // RG8
		{ VK_FORMAT_R8G8_SINT,                 VK_FORMAT_R8G8_SINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8I
		{ VK_FORMAT_R8G8_UINT,                 VK_FORMAT_R8G8_UINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8U
		{ VK_FORMAT_R8G8_SNORM,                VK_FORMAT_R8G8_SNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8S
		{ VK_FORMAT_R16G16_UNORM,              VK_FORMAT_R16G16_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16
		{ VK_FORMAT_R16G16_SINT,               VK_FORMAT_R16G16_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16I
		{ VK_FORMAT_R16G16_UINT,               VK_FORMAT_R16G16_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16U
		{ VK_FORMAT_R16G16_SFLOAT,             VK_FORMAT_R16G16_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16F
		{ VK_FORMAT_R16G16_SNORM,              VK_FORMAT_R16G16_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16S
		{ VK_FORMAT_R32G32_SINT,               VK_FORMAT_R32G32_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32I
		{ VK_FORMAT_R32G32_UINT,               VK_FORMAT_R32G32_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32U
		{ VK_FORMAT_R32G32_SFLOAT,             VK_FORMAT_R32G32_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32F
		{ VK_FORMAT_R8G8B8_UNORM,              VK_FORMAT_R8G8B8_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8
		{ VK_FORMAT_R8G8B8_SINT,               VK_FORMAT_R8G8B8_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8I
		{ VK_FORMAT_R8G8B8_UINT,               VK_FORMAT_R8G8B8_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8U
		{ VK_FORMAT_R8G8B8_SNORM,              VK_FORMAT_R8G8B8_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB8S
		{ VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,   VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB9E5F
		{ VK_FORMAT_B8G8R8A8_UNORM,            VK_FORMAT_B8G8R8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_B8G8R8A8_SRGB,            { $_, $_, $_, $_ } }, // BGRA8
		{ VK_FORMAT_R8G8B8A8_UNORM,            VK_FORMAT_R8G8B8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8
		{ VK_FORMAT_R8G8B8A8_SINT,             VK_FORMAT_R8G8B8A8_SINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8I
		{ VK_FORMAT_R8G8B8A8_UINT,             VK_FORMAT_R8G8B8A8_UINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8U
		{ VK_FORMAT_R8G8B8A8_SNORM,            VK_FORMAT_R8G8B8A8_SNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA8S
		{ VK_FORMAT_R16G16B16A16_UNORM,        VK_FORMAT_R16G16B16A16_UNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16
		{ VK_FORMAT_R16G16B16A16_SINT,         VK_FORMAT_R16G16B16A16_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16I
		{ VK_FORMAT_R16G16B16A16_UINT,         VK_FORMAT_R16G16B16A16_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16U
		{ VK_FORMAT_R16G16B16A16_SFLOAT,       VK_FORMAT_R16G16B16A16_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16F
		{ VK_FORMAT_R16G16B16A16_SNORM,        VK_FORMAT_R16G16B16A16_SNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16S
		{ VK_FORMAT_R32G32B32A32_SINT,         VK_FORMAT_R32G32B32A32_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32I
		{ VK_FORMAT_R32G32B32A32_UINT,         VK_FORMAT_R32G32B32A32_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32U
		{ VK_FORMAT_R32G32B32A32_SFLOAT,       VK_FORMAT_R32G32B32A32_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32F
		{ VK_FORMAT_R5G6B5_UNORM_PACK16,       VK_FORMAT_R5G6B5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // B5G6R5
		{ VK_FORMAT_B5G6R5_UNORM_PACK16,       VK_FORMAT_B5G6R5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R5G6B5
		{ VK_FORMAT_B4G4R4A4_UNORM_PACK16,     VK_FORMAT_B4G4R4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $G, $R, $A, $B } }, // BGRA4
		{ VK_FORMAT_R4G4B4A4_UNORM_PACK16,     VK_FORMAT_R4G4B4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $A, $B, $G, $R } }, // RGBA4
		{ VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BGR5A1
		{ VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB5A1
		{ VK_FORMAT_A2R10G10B10_UNORM_PACK32,  VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB10A2
		{ VK_FORMAT_B10G11R11_UFLOAT_PACK32,   VK_FORMAT_B10G11R11_UFLOAT_PACK32,  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG11B10F
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // UnknownDepth
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_D16_UNORM,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24S8
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16F
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24F
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32F
		{ VK_FORMAT_UNDEFINED,                 VK_FORMAT_R8_UINT,                  VK_FORMAT_S8_UINT,             VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D0S8
#undef $_
#undef $0
#undef $1
#undef $R
#undef $G
#undef $B
#undef $A
	};
	static_assert(bimg::TextureFormat::Count == std::size(k_texture_format_map));

    static constexpr std::size_t k_natural_align = 8;
    auto texture_allocator::realloc(void* p, std::size_t size, std::size_t align, const char*, std::uint32_t) -> void* {
        if (0 == size) {
            if (nullptr != p) {
                if (k_natural_align >= align) {
                    mi_free(p);
                    return nullptr;
                }
                mi_free_aligned(p, align);
            }
            return nullptr;
        }
        if (nullptr == p) {
            if (k_natural_align >= align) {
                return mi_malloc(size);
            }
            return mi_malloc_aligned(size, align);
        }
        if (k_natural_align >= align) {
            return mi_realloc(p, size);
        }
        return mi_realloc_aligned(p, size, align);
    }

    constinit texture_allocator s_texture_allocator {};

    texture::texture(std::string&& asset_path) : asset { asset_category::texture, asset_source::filesystem, std::move(asset_path) } {
        std::vector<std::uint8_t> texels {};
        assetmgr::load_asset_blob_or_panic(asset_category::texture, get_asset_path(), texels);
        parse_from_raw_memory(texels);
    }

    texture::texture(const std::span<const std::uint8_t> raw_mem) : asset {asset_category::texture, asset_source::memory} {
        parse_from_raw_memory(raw_mem);
    }

    texture::texture(
        const vk::ImageType type,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth,
        const std::uint32_t mip_levels,
        const std::uint32_t array_size,
        const vk::Format format,
        const VmaMemoryUsage memory_usage,
        const vk::ImageUsageFlags usage,
        const vk::SampleCountFlagBits sample_count,
        const vk::ImageLayout initial_layout,
        const void* data,
        const std::size_t size,
        const vk::ImageCreateFlags flags,
        const vk::ImageTiling tiling
    ) : asset {asset_category::texture, asset_source::memory} {
        create(
            type,
            width,
            height,
            depth,
            mip_levels,
            array_size,
            format,
            memory_usage,
            usage,
            sample_count,
            initial_layout,
            data,
            size,
            flags,
            tiling
        );
    }

    texture::~texture() {
        if (m_image) {
            vkb_vk_device().destroyImageView(m_image_view, &vkb::s_allocator);
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = nullptr;
        }
    }

    auto texture::create(
        const vk::ImageType type,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth,
        const std::uint32_t mip_levels,
        const std::uint32_t array_size,
        const vk::Format format,
        const VmaMemoryUsage memory_usage,
        const vk::ImageUsageFlags usage,
        const vk::SampleCountFlagBits sample_count,
        const vk::ImageLayout initial_layout,
        const void* data,
        const std::size_t size,
        const vk::ImageCreateFlags flags,
        const vk::ImageTiling tiling
    ) -> void {
        m_width = width;
        m_height = height;
        m_depth = depth;
        m_mip_levels = mip_levels ? mip_levels : static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        m_array_size = array_size;
        m_format = format;
        m_usage = usage;
        m_memory_usage = memory_usage;
        m_sample_count = sample_count;
        m_type = type;
        m_flags = flags;
        m_tiling = tiling;
        m_allocator = vkb_device().get_allocator();

        vk::ImageCreateInfo image_info {};
        image_info.imageType = m_type;
        image_info.extent.width = m_width;
        image_info.extent.height = m_height;
        image_info.extent.depth = m_depth;
        image_info.mipLevels = m_mip_levels;
        image_info.arrayLayers = m_array_size;
        image_info.format = m_format;
        image_info.tiling = m_tiling;
        image_info.initialLayout = initial_layout;
        image_info.usage = m_usage;
        image_info.samples = m_sample_count;
        image_info.sharingMode = vk::SharingMode::eExclusive;
        image_info.flags = m_flags;

        VmaAllocationInfo alloc_info {};
        VmaAllocationCreateInfo alloc_create_info {};

        alloc_create_info.usage = m_memory_usage;
        alloc_create_info.flags = memory_usage == VMA_MEMORY_USAGE_CPU_ONLY || memory_usage == VMA_MEMORY_USAGE_GPU_TO_CPU ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;
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

        if (data && size) [[likely]] {
            const vk::CommandBuffer copy_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eTransfer>();

            vk::ImageSubresourceRange subresource_range {};
            subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
            subresource_range.baseMipLevel = 0;
            subresource_range.baseArrayLayer = 0;
            subresource_range.levelCount = m_mip_levels;
            subresource_range.layerCount = m_array_size;

            // Image barrier for optimal image (target)
            // Optimal image will be used as destination for the copy
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );

            vkb_context().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);

            upload(0, 0, data, size, vk::ImageLayout::eTransferDstOptimal);

            if (m_mip_levels > 0) {
                // if mip_levels == 1, we can just use the image as-is and not generate mipmaps but only barrier them
                const bool transfer_only = mip_levels > 1;
                generate_mips(transfer_only);
            }
        }

        // image view:
        vk::ImageViewCreateInfo image_view_ci {};
        image_view_ci.viewType = vk::ImageViewType::e2D;
        image_view_ci.format = m_format;
        image_view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_ci.subresourceRange.baseMipLevel = 0;
        image_view_ci.subresourceRange.baseArrayLayer = 0;
        image_view_ci.subresourceRange.layerCount = m_array_size;
        image_view_ci.subresourceRange.levelCount = m_mip_levels;
        image_view_ci.image = m_image;
        vkcheck(vkb_vk_device().createImageView(&image_view_ci, &vkb::s_allocator, &m_image_view));
    }

    auto texture::parse_from_raw_memory(const std::span<const std::uint8_t> texels) -> void {
        passert(texels.size() <= std::numeric_limits<std::uint32_t>::max());
        bimg::ImageContainer* image = bimg::imageParse(&s_texture_allocator, texels.data(), static_cast<std::uint32_t>(texels.size()));
        passert(image != nullptr);

        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
        constexpr vk::ImageCreateFlags create_flags {};
        constexpr vk::ImageTiling tiling = vk::ImageTiling::eOptimal;

        auto format = static_cast<vk::Format>(k_texture_format_map[image->m_format].fmt);
        if (!vkb_device().is_image_format_supported(vk::ImageType::e2D, format, create_flags, usage, tiling)) [[unlikely]] {
            log_warn("Texture format not supported: {}, converting...", string_VkFormat(static_cast<VkFormat>(format)));
            bimg::ImageContainer* original = image;
            image = bimg::imageConvert(&s_texture_allocator, k_fallback_format, *original, true);
            bimg::imageFree(original);
            passert(image != nullptr);
            format = static_cast<vk::Format>(k_texture_format_map[k_fallback_format].fmt);
        }
        create(
            vk::ImageType::e2D, // TODO: cubemap?
            image->m_width,
            image->m_height,
            image->m_depth,
            image->m_numMips <= 1 ? 0 : image->m_numMips,
            image->m_numLayers,
            format,
            VMA_MEMORY_USAGE_GPU_ONLY,
            usage,
            vk::SampleCountFlagBits::e1,
            vk::ImageLayout::eUndefined,
            image->m_data,
            image->m_size,
            create_flags,
            tiling
        );

        m_approx_byte_size = sizeof(*this) + image->m_size;

        bimg::imageFree(image);
    }

    auto texture::upload(
        const std::size_t array_idx,
        const std::size_t mip_level,
        const void* data,
        const std::size_t size,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout
    ) const -> void {
        vkb::buffer staging {};
        staging.create(size, 0, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, data);

        vk::BufferImageCopy region {};
        region.bufferOffset = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = mip_level;
        region.imageSubresource.baseArrayLayer = array_idx;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = m_width;
        region.imageExtent.height = m_height;
        region.imageExtent.depth = 1;

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = mip_level;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;
        subresource_range.baseArrayLayer = array_idx;

        const vk::CommandBuffer copy_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eTransfer>();

        if (src_layout != vk::ImageLayout::eTransferDstOptimal) {
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                src_layout,
                vk::ImageLayout::eTransferDstOptimal,
                subresource_range
            );
        }
        copy_cmd.copyBufferToImage(
            staging.get_buffer(),
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &region
        );
        if (dst_layout != vk::ImageLayout::eTransferDstOptimal) {
            set_image_layout_barrier(
                copy_cmd,
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                dst_layout,
                subresource_range
            );
        }

        vkb_context().flush_command_buffer<vk::QueueFlagBits::eTransfer>(copy_cmd);
    }

    auto texture::generate_mips(
        const bool transfer_only,
        const vk::ImageLayout src_layout,
        const vk::ImageLayout dst_layout,
        const vk::ImageAspectFlags aspect_mask,
        const vk::Filter filter
    ) const -> void {
        const vk::CommandBuffer blit_cmd = vkb_context().start_command_buffer<vk::QueueFlagBits::eGraphics>();

        vk::ImageSubresourceRange intial_subresource_range {};
        intial_subresource_range.aspectMask = aspect_mask;
        intial_subresource_range.baseMipLevel = 1;
        intial_subresource_range.levelCount = m_mip_levels - 1;
        intial_subresource_range.layerCount = m_array_size;
        intial_subresource_range.baseArrayLayer = 0;

        set_image_layout_barrier(
            blit_cmd,
            m_image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            intial_subresource_range
        );

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = aspect_mask;
        subresource_range.levelCount = 1;
        subresource_range.layerCount = 1;

        for (std::uint32_t i = 0; i < m_array_size; ++i) {
            std::uint32_t mip_w = m_width;
            std::uint32_t mip_h = m_height;
            for (std::uint32_t j = 1; j < m_mip_levels; ++j) {
                subresource_range.baseMipLevel = j - 1;
                subresource_range.baseArrayLayer = i;
                vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal;
                if (j == 1) {
                    layout = src_layout;
                }
                if (layout != vk::ImageLayout::eTransferSrcOptimal) {
                    set_image_layout_barrier(
                        blit_cmd,
                        m_image,
                        layout,
                        vk::ImageLayout::eTransferSrcOptimal,
                        subresource_range
                    );
                }
                if (!transfer_only) { // if we're only doing a transfer, we don't need to blit
                    vk::ImageBlit blit {};
                    blit.srcOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.srcOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w), static_cast<std::int32_t>(mip_h), 1 };
                    blit.srcSubresource.aspectMask = aspect_mask;
                    blit.srcSubresource.mipLevel = j - 1;
                    blit.srcSubresource.baseArrayLayer = i;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = vk::Offset3D { 0, 0, 0 };
                    blit.dstOffsets[1] = vk::Offset3D { static_cast<std::int32_t>(mip_w > 1 ? mip_w >> 1 : 1), static_cast<std::int32_t>(mip_h > 1 ? mip_h >> 1 : 1), 1 };
                    blit.dstSubresource.aspectMask = aspect_mask;
                    blit.dstSubresource.mipLevel = j;
                    blit.dstSubresource.baseArrayLayer = i;
                    blit.dstSubresource.layerCount = 1;
                    blit_cmd.blitImage(
                        m_image,
                        vk::ImageLayout::eTransferSrcOptimal,
                        m_image,
                        vk::ImageLayout::eTransferDstOptimal,
                        1,
                        &blit,
                        filter
                    );
                }
                set_image_layout_barrier(
                    blit_cmd,
                    m_image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    dst_layout,
                    subresource_range
                );
                if (mip_w > 1) {
                    mip_w >>= 1;
                }
                if (mip_h > 1) {
                    mip_h >>= 1;
                }
            }
        }

        subresource_range.baseMipLevel = m_mip_levels - 1;
        set_image_layout_barrier(
            blit_cmd,
            m_image,
            vk::ImageLayout::eTransferDstOptimal,
            dst_layout,
            subresource_range
        );

        vkb_context().flush_command_buffer<vk::QueueFlagBits::eGraphics>(blit_cmd);
        log_info("{} mipchain with {} maps", transfer_only ? "Loaded" : "Generated", m_mip_levels);
    }

    auto texture::set_image_layout_barrier(
        const vk::CommandBuffer cmd_buf,
        const vk::Image image,
        const vk::ImageLayout old_layout,
        const vk::ImageLayout new_layout,
        const vk::ImageSubresourceRange range,
        const vk::PipelineStageFlags src_stage,
        const vk::PipelineStageFlags dst_stage
    ) -> void {
        vk::ImageMemoryBarrier barrier {};
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = image;
        barrier.subresourceRange = range;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (old_layout) {
            case vk::ImageLayout::eUndefined: barrier.srcAccessMask = {}; break;
            case vk::ImageLayout::ePreinitialized: barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eTransferDstOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eShaderReadOnlyOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead; break;
            default:
                panic("unsupported layout transition!");
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (new_layout) {
            case vk::ImageLayout::eTransferDstOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.dstAccessMask = barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                if (barrier.srcAccessMask == vk::AccessFlagBits {}) {
                    barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            break;
            default:
                panic("unsupported layout transition!");
        }

        cmd_buf.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
    }
}
