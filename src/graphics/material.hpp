// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/descriptor.hpp"
#include "../assetmgr/assetmgr.hpp"
#include "../scripting/system_variable.hpp"


#include "texture.hpp"

namespace lu::graphics {
    static const system_variable<eastl::string> sv_error_texture {"renderer.error_texture", eastl::monostate{}};
    static const system_variable<eastl::string> sv_fallback_image_white {"renderer.fallback_texture_w", eastl::monostate{}};
    static const system_variable<eastl::string> sv_fallback_image_black {"renderer.fallback_texture_b", eastl::monostate{}};

    using material_key = eastl::string;
    struct material_property final {
        constexpr material_property() noexcept = default;
        constexpr material_property(const std::uint32_t shader_binding, const assetmgr::asset_ref value)
            noexcept : shader_binding{shader_binding}, value{value} {}

        std::uint32_t shader_binding = 0;
        assetmgr::asset_ref value {};
    };

    class material : public assetmgr::asset {
    public:
        struct static_resources : public no_copy, public no_move {
            static_resources();
            ~static_resources();

            vkb::descriptor_allocator descriptor_allocator {};
            vkb::descriptor_layout_cache descriptor_layout_cache {};
            vk::DescriptorSetLayout descriptor_layout {};
        };

        explicit material(ankerl::unordered_dense::map<material_key, material_property>&& properties = {});
        ~material() override;

        ankerl::unordered_dense::map<material_key, material_property> properties {};

        auto print_properties() const -> void;

        [[nodiscard]] auto get_descriptor_set() const noexcept -> const vk::DescriptorSet& { return m_descriptor_set; }
        auto flush_property_updates() -> void;
        [[nodiscard]] static auto get_static_resources() -> const static_resources&;

    private:
        friend class graphics_subsystem;
        static auto init_static_resources() -> void;
        static auto free_static_resources() -> void;

        vk::DescriptorSet m_descriptor_set {};
    };
}
