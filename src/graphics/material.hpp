// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "../assetmgr/assetmgr.hpp"

#include "texture.hpp"

namespace graphics {
    class material : public asset {
    public:
        texture* albedo_map = nullptr;
        texture* normal_map = nullptr;
        texture* metallic_roughness_map = nullptr;
        texture* ambient_occlusion_map = nullptr;

        explicit material(
            texture* albedo_map = nullptr,
            texture* metallic_roughness_map = nullptr,
            texture* normal_map = nullptr,
            texture* ambient_occlusion_map = nullptr
        );
        ~material() override;

        [[nodiscard]] auto get_descriptor_set() const noexcept -> const vk::DescriptorSet& { return m_descriptor_set; }

        auto flush_property_updates() const -> void;

        [[nodiscard]] static auto get_error_texture() noexcept -> graphics::texture* {
            assert(s_error_texture.has_value());
            return &*s_error_texture;
        }

        [[nodiscard]] static auto get_flat_normal_map() noexcept -> graphics::texture* {
            assert(s_flat_normal.has_value());
            return &*s_flat_normal;
        }

    private:
        friend class graphics_subsystem;
        static auto init_static_resources() -> void;
        static auto free_static_resources() -> void;

        [[nodiscard]] static auto get_descriptor_set_layout() noexcept -> const vk::DescriptorSetLayout& {
            return s_descriptor_set_layout;
        }

        static inline constinit std::optional<graphics::texture> s_error_texture {};
        static inline constinit std::optional<graphics::texture> s_flat_normal {};
        static inline constinit vk::DescriptorPool s_descriptor_pool {};
        static inline constinit vk::DescriptorSetLayout s_descriptor_set_layout {};
        vk::DescriptorSet m_descriptor_set {};
    };
}
