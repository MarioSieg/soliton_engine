// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/descriptor.hpp"
#include "../assetmgr/assetmgr.hpp"

#include "texture.hpp"

namespace lu::graphics {
    class material : public assetmgr::asset {
    public:
        struct static_resources : public no_copy, public no_move {
            static_resources();
            ~static_resources();

            texture error_texture;
            texture flat_normal;
            texture flat_heightmap;
            vkb::descriptor_allocator descriptor_allocator {};
            vkb::descriptor_layout_cache descriptor_layout_cache {};
            vk::DescriptorSetLayout descriptor_layout {};
        };

        texture* albedo_map = nullptr;
        texture* normal_map = nullptr;
        texture* metallic_roughness_map = nullptr;
        texture* height_map = nullptr;
        texture* ambient_occlusion_map = nullptr;

        explicit material(
            texture* albedo_map = nullptr,
            texture* metallic_roughness_map = nullptr,
            texture* normal_map = nullptr,
            texture* height_map = nullptr,
            texture* ambient_occlusion_map = nullptr
        );
        ~material() override;

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
