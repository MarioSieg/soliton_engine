// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "../assetmgr/assetmgr.hpp"

#include "texture.hpp"

namespace graphics {
    class material : public asset {
    public:
        texture* albedo_map = nullptr;
        texture* metallic_roughness_map = nullptr;
        texture* normal_map = nullptr;
        texture* ambient_occlusion_map = nullptr;

        explicit material(
            texture* albedo_map = nullptr,
            texture* metallic_roughness_map = nullptr,
            texture* normal_map = nullptr,
            texture* ambient_occlusion_map = nullptr
        );
        ~material() override;

        [[nodiscard]] auto get_descriptor_set() const noexcept -> const vk::DescriptorSet& { return m_descriptor_set; }
        [[nodiscard]] auto get_sampler() const noexcept -> const vk::Sampler& { return m_sampler; }

        auto flush_property_updates() const -> void;
        static auto create_global_descriptors() -> void;
        static auto get_descriptor_set_layout() noexcept -> const vk::DescriptorSetLayout& {
            return m_descriptor_set_layout;
        }
    private:
        auto create_sampler() -> void;

        static inline constinit std::unique_ptr<graphics::texture> m_default_texture {};
        static inline constinit vk::DescriptorPool m_descriptor_pool {};
        static inline constinit vk::DescriptorSetLayout m_descriptor_set_layout {};
        vk::Sampler m_sampler {};
        vk::DescriptorSet m_descriptor_set {};
    };
}
