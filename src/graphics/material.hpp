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

        material(
            texture* albedo_map,
            texture* metallic_roughness_map,
            texture* normal_map,
            texture* ambient_occlusion_map
        );
        ~material() override;

        auto flush_property_updates() const -> void;
        static auto create_descriptor_set_layout_lazy() -> void;
        static auto get_descriptor_set_layout() noexcept -> vk::DescriptorSetLayout {
            return m_descriptor_set_layout;
        }
    private:
        auto create_sampler() -> void;

        static inline constinit vk::DescriptorSetLayout m_descriptor_set_layout {};
        vk::Sampler m_sampler {};
        vk::DescriptorSet m_descriptor_set {};
    };
}
