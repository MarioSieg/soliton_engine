// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "texture_loader.hpp"

#include <bx/allocator.h>
#include <bimg/bimg.h>
#include <bimg/decode.h>
#include <Simd/SimdLib.hpp>

#include "../vulkancore/context.hpp"

namespace lu::graphics {
    // Textures are converted to this format when native format is not supported on the GPU
    static constexpr bimg::TextureFormat::Enum k_fallback_format = bimg::TextureFormat::RGBA8;

    struct texture_format_info final {
        VkFormat fmt {};
        VkFormat fmt_srv {};
        VkFormat fmt_dsv {};
        VkFormat fmt_srgb {};
        VkComponentMapping mapping {};
    };

    static constexpr texture_format_info k_texture_format_map[] {
        #define $_ VK_COMPONENT_SWIZZLE_IDENTITY
        #define $0 VK_COMPONENT_SWIZZLE_ZERO
        #define $1 VK_COMPONENT_SWIZZLE_ONE
        #define $R VK_COMPONENT_SWIZZLE_R
        #define $G VK_COMPONENT_SWIZZLE_G
        #define $B VK_COMPONENT_SWIZZLE_B
        #define $A VK_COMPONENT_SWIZZLE_A
        texture_format_info { VK_FORMAT_BC1_RGB_UNORM_BLOCK,       VK_FORMAT_BC1_RGB_UNORM_BLOCK,      VK_FORMAT_UNDEFINED,           VK_FORMAT_BC1_RGB_SRGB_BLOCK,       { $_, $_, $_, $_ } }, // BC1
        texture_format_info { VK_FORMAT_BC2_UNORM_BLOCK,           VK_FORMAT_BC2_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC2_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC2
        texture_format_info { VK_FORMAT_BC3_UNORM_BLOCK,           VK_FORMAT_BC3_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC3_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC3
        texture_format_info { VK_FORMAT_BC4_UNORM_BLOCK,           VK_FORMAT_BC4_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC4
        texture_format_info { VK_FORMAT_BC5_UNORM_BLOCK,           VK_FORMAT_BC5_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC5
        texture_format_info { VK_FORMAT_BC6H_SFLOAT_BLOCK,         VK_FORMAT_BC6H_SFLOAT_BLOCK,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BC6H
        texture_format_info { VK_FORMAT_BC7_UNORM_BLOCK,           VK_FORMAT_BC7_UNORM_BLOCK,          VK_FORMAT_UNDEFINED,           VK_FORMAT_BC7_SRGB_BLOCK,           { $_, $_, $_, $_ } }, // BC7
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ETC1
        texture_format_info { VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,   { $_, $_, $_, $_ } }, // ETC2
        texture_format_info { VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A
        texture_format_info { VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, { $_, $_, $_, $_ } }, // ETC2A1
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC12A
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC14A
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC22
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // PTC24
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATC
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCE
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // ATCI
        texture_format_info { VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      VK_FORMAT_ASTC_4x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_4x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC4x4
        texture_format_info { VK_FORMAT_ASTC_5x4_UNORM_BLOCK,      VK_FORMAT_ASTC_5x4_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x4_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x4
        texture_format_info { VK_FORMAT_ASTC_5x5_UNORM_BLOCK,      VK_FORMAT_ASTC_5x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_5x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC5x5
        texture_format_info { VK_FORMAT_ASTC_6x5_UNORM_BLOCK,      VK_FORMAT_ASTC_6x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x5
        texture_format_info { VK_FORMAT_ASTC_6x6_UNORM_BLOCK,      VK_FORMAT_ASTC_6x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_6x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC6x6
        texture_format_info { VK_FORMAT_ASTC_8x5_UNORM_BLOCK,      VK_FORMAT_ASTC_8x5_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x5_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x5
        texture_format_info { VK_FORMAT_ASTC_8x6_UNORM_BLOCK,      VK_FORMAT_ASTC_8x6_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x6_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x6
        texture_format_info { VK_FORMAT_ASTC_8x8_UNORM_BLOCK,      VK_FORMAT_ASTC_8x8_UNORM_BLOCK,     VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_8x8_SRGB_BLOCK,      { $_, $_, $_, $_ } }, // ASTC8x8
        texture_format_info { VK_FORMAT_ASTC_10x5_UNORM_BLOCK,     VK_FORMAT_ASTC_10x5_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x5_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x5
        texture_format_info { VK_FORMAT_ASTC_10x6_UNORM_BLOCK,     VK_FORMAT_ASTC_10x6_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x6_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x6
        texture_format_info { VK_FORMAT_ASTC_10x8_UNORM_BLOCK,     VK_FORMAT_ASTC_10x8_UNORM_BLOCK,    VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x8_SRGB_BLOCK,     { $_, $_, $_, $_ } }, // ASTC10x8
        texture_format_info { VK_FORMAT_ASTC_10x10_UNORM_BLOCK,    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_10x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC10x10
        texture_format_info { VK_FORMAT_ASTC_12x10_UNORM_BLOCK,    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x10_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x10
        texture_format_info { VK_FORMAT_ASTC_12x12_UNORM_BLOCK,    VK_FORMAT_ASTC_12x12_UNORM_BLOCK,   VK_FORMAT_UNDEFINED,           VK_FORMAT_ASTC_12x12_SRGB_BLOCK,    { $_, $_, $_, $_ } }, // ASTC12x12
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // Unknown
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R1
        texture_format_info { VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $0, $0, $0, $R } }, // A8
        texture_format_info { VK_FORMAT_R8_UNORM,                  VK_FORMAT_R8_UNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_R8_SRGB,                  { $_, $_, $_, $_ } }, // R8
        texture_format_info { VK_FORMAT_R8_SINT,                   VK_FORMAT_R8_SINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8I
        texture_format_info { VK_FORMAT_R8_UINT,                   VK_FORMAT_R8_UINT,                  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8U
        texture_format_info { VK_FORMAT_R8_SNORM,                  VK_FORMAT_R8_SNORM,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R8S
        texture_format_info { VK_FORMAT_R16_UNORM,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16
        texture_format_info { VK_FORMAT_R16_SINT,                  VK_FORMAT_R16_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16I
        texture_format_info { VK_FORMAT_R16_UINT,                  VK_FORMAT_R16_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16U
        texture_format_info { VK_FORMAT_R16_SFLOAT,                VK_FORMAT_R16_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16F
        texture_format_info { VK_FORMAT_R16_SNORM,                 VK_FORMAT_R16_SNORM,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R16S
        texture_format_info { VK_FORMAT_R32_SINT,                  VK_FORMAT_R32_SINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32I
        texture_format_info { VK_FORMAT_R32_UINT,                  VK_FORMAT_R32_UINT,                 VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32U
        texture_format_info { VK_FORMAT_R32_SFLOAT,                VK_FORMAT_R32_SFLOAT,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R32F
        texture_format_info { VK_FORMAT_R8G8_UNORM,                VK_FORMAT_R8G8_UNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8_SRGB,                { $_, $_, $_, $_ } }, // RG8
        texture_format_info { VK_FORMAT_R8G8_SINT,                 VK_FORMAT_R8G8_SINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8I
        texture_format_info { VK_FORMAT_R8G8_UINT,                 VK_FORMAT_R8G8_UINT,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8U
        texture_format_info { VK_FORMAT_R8G8_SNORM,                VK_FORMAT_R8G8_SNORM,               VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG8S
        texture_format_info { VK_FORMAT_R16G16_UNORM,              VK_FORMAT_R16G16_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16
        texture_format_info { VK_FORMAT_R16G16_SINT,               VK_FORMAT_R16G16_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16I
        texture_format_info { VK_FORMAT_R16G16_UINT,               VK_FORMAT_R16G16_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16U
        texture_format_info { VK_FORMAT_R16G16_SFLOAT,             VK_FORMAT_R16G16_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16F
        texture_format_info { VK_FORMAT_R16G16_SNORM,              VK_FORMAT_R16G16_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG16S
        texture_format_info { VK_FORMAT_R32G32_SINT,               VK_FORMAT_R32G32_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32I
        texture_format_info { VK_FORMAT_R32G32_UINT,               VK_FORMAT_R32G32_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32U
        texture_format_info { VK_FORMAT_R32G32_SFLOAT,             VK_FORMAT_R32G32_SFLOAT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG32F
        texture_format_info { VK_FORMAT_R8G8B8_UNORM,              VK_FORMAT_R8G8B8_UNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8
        texture_format_info { VK_FORMAT_R8G8B8_SINT,               VK_FORMAT_R8G8B8_SINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8I
        texture_format_info { VK_FORMAT_R8G8B8_UINT,               VK_FORMAT_R8G8B8_UINT,              VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8_SRGB,              { $_, $_, $_, $_ } }, // RGB8U
        texture_format_info { VK_FORMAT_R8G8B8_SNORM,              VK_FORMAT_R8G8B8_SNORM,             VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB8S
        texture_format_info { VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,   VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGB9E5F
        texture_format_info { VK_FORMAT_B8G8R8A8_UNORM,            VK_FORMAT_B8G8R8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_B8G8R8A8_SRGB,            { $_, $_, $_, $_ } }, // BGRA8
        texture_format_info { VK_FORMAT_R8G8B8A8_UNORM,            VK_FORMAT_R8G8B8A8_UNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8
        texture_format_info { VK_FORMAT_R8G8B8A8_SINT,             VK_FORMAT_R8G8B8A8_SINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8I
        texture_format_info { VK_FORMAT_R8G8B8A8_UINT,             VK_FORMAT_R8G8B8A8_UINT,            VK_FORMAT_UNDEFINED,           VK_FORMAT_R8G8B8A8_SRGB,            { $_, $_, $_, $_ } }, // RGBA8U
        texture_format_info { VK_FORMAT_R8G8B8A8_SNORM,            VK_FORMAT_R8G8B8A8_SNORM,           VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA8S
        texture_format_info { VK_FORMAT_R16G16B16A16_UNORM,        VK_FORMAT_R16G16B16A16_UNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16
        texture_format_info { VK_FORMAT_R16G16B16A16_SINT,         VK_FORMAT_R16G16B16A16_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16I
        texture_format_info { VK_FORMAT_R16G16B16A16_UINT,         VK_FORMAT_R16G16B16A16_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16U
        texture_format_info { VK_FORMAT_R16G16B16A16_SFLOAT,       VK_FORMAT_R16G16B16A16_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16F
        texture_format_info { VK_FORMAT_R16G16B16A16_SNORM,        VK_FORMAT_R16G16B16A16_SNORM,       VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA16S
        texture_format_info { VK_FORMAT_R32G32B32A32_SINT,         VK_FORMAT_R32G32B32A32_SINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32I
        texture_format_info { VK_FORMAT_R32G32B32A32_UINT,         VK_FORMAT_R32G32B32A32_UINT,        VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32U
        texture_format_info { VK_FORMAT_R32G32B32A32_SFLOAT,       VK_FORMAT_R32G32B32A32_SFLOAT,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RGBA32F
        texture_format_info { VK_FORMAT_R5G6B5_UNORM_PACK16,       VK_FORMAT_R5G6B5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // B5G6R5
        texture_format_info { VK_FORMAT_B5G6R5_UNORM_PACK16,       VK_FORMAT_B5G6R5_UNORM_PACK16,      VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // R5G6B5
        texture_format_info { VK_FORMAT_B4G4R4A4_UNORM_PACK16,     VK_FORMAT_B4G4R4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $G, $R, $A, $B } }, // BGRA4
        texture_format_info { VK_FORMAT_R4G4B4A4_UNORM_PACK16,     VK_FORMAT_R4G4B4A4_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $A, $B, $G, $R } }, // RGBA4
        texture_format_info { VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // BGR5A1
        texture_format_info { VK_FORMAT_A1R5G5B5_UNORM_PACK16,     VK_FORMAT_A1R5G5B5_UNORM_PACK16,    VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB5A1
        texture_format_info { VK_FORMAT_A2R10G10B10_UNORM_PACK32,  VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $B, $G, $R, $A } }, // RGB10A2
        texture_format_info { VK_FORMAT_B10G11R11_UFLOAT_PACK32,   VK_FORMAT_B10G11R11_UFLOAT_PACK32,  VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // RG11B10F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_UNDEFINED,                VK_FORMAT_UNDEFINED,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // UnknownDepth
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R16_UNORM,                VK_FORMAT_D16_UNORM,           VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24S8
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT_S8_UINT,  VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D16F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D24F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R32_SFLOAT,               VK_FORMAT_D32_SFLOAT,          VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D32F
        texture_format_info { VK_FORMAT_UNDEFINED,                 VK_FORMAT_R8_UINT,                  VK_FORMAT_S8_UINT,             VK_FORMAT_UNDEFINED,                { $_, $_, $_, $_ } }, // D0S8
        #undef $_
        #undef $0
        #undef $1
        #undef $R
        #undef $G
        #undef $B
        #undef $A
    };
    static_assert(bimg::TextureFormat::Count == std::size(k_texture_format_map));

    /*
     * Enables SIMD conversion for common formats (RGB8 -> RGBA8) using runtime CPU detection and vectorized algorithms.
     * This makes the importing of external scenes with raw .png or .jpg images noteably faster.
     * Example (a big interior scene with some raw images) (RELEASE):
     * ON: 1.98S
     * OFF: 2.54s
     * In DEBUG mode the difference is even bigger, which allows for faster iteration times:
     * ON: 6.19s
     * OFF: 18.2s
     */
    static constexpr bool k_enable_simd_cvt = true;

    class texture_allocator final : public bx::AllocatorI {
    public:
        auto realloc(void* p, size_t size, size_t align, const char* filePath, std::uint32_t line) -> void* override;
    };

#if USE_MIMALLOC
    static constexpr std::size_t k_natural_align = 8;
    auto texture_allocator::realloc(void* p, const std::size_t size, const std::size_t align, const char*, std::uint32_t) -> void* {
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
#else
    static bx::DefaultAllocator s_def_texture_allocator {};
#endif

    [[nodiscard]] consteval auto get_tex_alloc() noexcept -> bx::AllocatorI* {
#if USE_MIMALLOC
        return &s_texture_allocator;
#else
        return &s_def_texture_allocator;
#endif
    }

    static constexpr vk::ImageUsageFlags k_def_usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
    static constexpr vk::ImageTiling k_def_tiling = vk::ImageTiling::eOptimal;

    [[nodiscard]] static auto convert_rgb8_to_rgba8_cpu_simd(
        bimg::ImageContainer*& image,
        bimg::ImageContainer* original,
        const bool is_cubemap
    ) -> bool {
        static_assert(k_fallback_format == bimg::TextureFormat::RGBA8, "Fallback format must be RGBA8 for SIMD conversion");
        image = bimg::imageAlloc(
            get_tex_alloc(),
            k_fallback_format,
            original->m_width,
            original->m_height,
            original->m_depth,
            original->m_numLayers,
            is_cubemap,
            original->m_numMips > 1
        );
        if (!image) [[unlikely]] {
            log_error("Failed to allocate image for SIMD conversion");
            return false;
        }
        const std::uint16_t sides = original->m_numLayers * (is_cubemap ? 6 : 1);
        for (std::uint16_t side = 0; side < sides; ++side) {
            for (std::uint8_t lod = 0, num = original->m_numMips; lod < num; ++lod) {
                bimg::ImageMip src_mip {};
                if (bimg::imageGetRawData(*original, side, lod, original->m_data, original->m_size, src_mip)) [[likely]] {
                    bimg::ImageMip dst_mip {};
                    bimg::imageGetRawData(*image, side, lod, image->m_data, image->m_size, dst_mip);
                    const std::uint8_t* const src = src_mip.m_data;
                    auto* const dst = const_cast<std::uint8_t*>(dst_mip.m_data);
                    SimdRgbToBgra(
                        src,
                        src_mip.m_width,
                        src_mip.m_height,
                        src_mip.m_width * 3,
                        dst,
                        dst_mip.m_width * 4,
                        0xff
                    );
                    SimdBgraToRgba(
                        dst,
                        dst_mip.m_width,
                        dst_mip.m_height,
                        dst_mip.m_width * 4,
                        dst,
                        dst_mip.m_width * 4
                    );
                } else {
                    log_warn("Failed to get raw data for mip level {}", lod);
                }
            }
        }
        return true;
    }

    static auto convert_texture_to_fallback_format_cpu(
        bimg::ImageContainer*& image,
        vk::Format& format,
        const bool is_cubemap
    ) -> void {
        const auto now = eastl::chrono::high_resolution_clock::now();
        bimg::ImageContainer* original = image;
        if (k_enable_simd_cvt && original->m_format == bimg::TextureFormat::RGB8) { // User faster SIMD for conversion of common formats
            if (!convert_rgb8_to_rgba8_cpu_simd(image, original, is_cubemap)) [[unlikely]] {
                log_error("Failed to convert image to fallback format using SIMD");
                return;
            }
        } else {
            image = bimg::imageConvert(get_tex_alloc(), k_fallback_format, *original, true);
            if (!image) [[unlikely]] {
                log_error("Failed to convert image to fallback format");
                return;
            }
        }
        bimg::imageFree(original);
        original = nullptr;
        log_warn(
            "Texture format not supported by GPU: {} converted to fallback format {} in {:.03f}ms",
            string_VkFormat(static_cast<VkFormat>(format)),
            string_VkFormat(k_texture_format_map[k_fallback_format].fmt),
            eastl::chrono::duration_cast<eastl::chrono::duration<double, std::milli>>(eastl::chrono::high_resolution_clock::now() - now).count()
        );
        format = static_cast<vk::Format>(k_texture_format_map[k_fallback_format].fmt);
    }

    static auto construct_image_copy_regions_generic(
        const bimg::ImageContainer& image,
        const texture_descriptor& desc,
        texture_data_supplier& data
    ) -> void {
        const std::uint32_t n =
            desc.mipmap_mode == texture_descriptor::mipmap_creation_mode::present_in_data
            ? desc.miplevel_count : 1;
        data.mip_copy_regions.reserve(n);
        std::size_t offset = 0;
        const auto fetch = [&]<const bool is_cube>(const std::uint32_t lod, const std::uint32_t face) -> void {
            bimg::ImageMip mip {};
            if (!bimg::imageGetRawData(image, is_cube ? face : 0, lod, data.data, data.size, mip)) [[unlikely]] {
                log_warn("Failed to fetch texture raw data for mip level: {}, face: {}", lod, face);
                return;
            }
            const std::size_t moff = offset;
            offset += mip.m_size;

            vk::BufferImageCopy region {};
            region.bufferOffset = moff;
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = lod;
            region.imageSubresource.baseArrayLayer = is_cube ? face : 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = std::max(1u, desc.width >> lod);
            region.imageExtent.height = std::max(1u, desc.height >> lod);
            region.imageExtent.depth = 1;
            data.mip_copy_regions.emplace_back(region);
        };
        if (desc.is_cubemap)
            for (std::uint32_t face = 0; face < desc.array_size; ++face)
                for (std::uint32_t i = 0; i < n; ++i)
                    fetch.operator()<true>(i, face);
        else
            for (std::uint32_t i = 0; i < n; ++i)
                fetch.operator()<false>(i, 0);
    }

    [[nodiscard]] static auto raw_parse_texture_generic(
        const eastl::span<const std::byte> buf,
        const eastl::function<auto (const texture_descriptor& info, const texture_data_supplier& data) -> void>& callback
    ) -> bool {
        if (buf.empty() || buf.size() >= eastl::numeric_limits<std::uint32_t>::max()) [[unlikely]] {
            log_error("Empty buffer, or buffer too large");
            return false;
        }

        bimg::ImageContainer* image = bimg::imageParse(
            get_tex_alloc(),
            buf.data(),
            static_cast<std::uint32_t>(buf.size())
        );
        if (!image) [[unlikely]] {
            log_error("Failed to parse image");
            return false;
        }

        vk::ImageCreateFlags create_flags {};
        const bool is_cubemap = image->m_cubeMap;
        if (is_cubemap) create_flags |= vk::ImageCreateFlagBits::eCubeCompatible;

        auto format = static_cast<vk::Format>(k_texture_format_map[image->m_format].fmt);

        bool is_format_supported = false;
        bool is_format_mipgen_supported = false;
        vkb::dvc().is_image_format_supported(
            vk::ImageType::e2D,
            format,
            create_flags,
            k_def_usage,
            k_def_tiling,
            is_format_supported,
            is_format_mipgen_supported
        );

        if (!is_cubemap && (!is_format_supported || (image->m_numMips <= 1 && !is_format_mipgen_supported)))
            convert_texture_to_fallback_format_cpu(image, format, is_cubemap);

        const texture_descriptor info {
            .width = image->m_width,
            .height = image->m_height,
            .depth = image->m_depth,
            .miplevel_count = image->m_numMips <= 1 ? texture::get_mip_count(image->m_width, image->m_height) : image->m_numMips,
            .mipmap_mode = !is_cubemap && image->m_numMips <= 1 ? texture_descriptor::mipmap_creation_mode::generate : texture_descriptor::mipmap_creation_mode::present_in_data,
            .array_size = is_cubemap ? 6u : image->m_numLayers,
            .format = format,
            .memory_usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .usage = k_def_usage,
            .sample_count = vk::SampleCountFlagBits::e1,
            .initial_layout = vk::ImageLayout::eUndefined,
            .tiling = k_def_tiling,
            .type = vk::ImageType::e2D,
            .flags = create_flags,
            .is_cubemap = is_cubemap,
        };

        texture_data_supplier data {
            .data = image->m_data,
            .size = image->m_size
        };

        construct_image_copy_regions_generic(*image, info, data);

        eastl::invoke(callback, info, data);

        const exit_guard img_free {
            [&image] () -> void {
                bimg::imageFree(image);
                image = nullptr;
            }
        };
        return true;
    }

    auto raw_parse_texture(
        const eastl::span<const std::byte> buf,
        const eastl::function<auto (const texture_descriptor& info, const texture_data_supplier& data) -> void>& callback
    ) -> bool {
        return raw_parse_texture_generic(buf, callback);
    }
}
