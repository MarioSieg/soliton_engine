// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../graphics_pipeline.hpp"
#include "../texture.hpp"
#include "../mesh.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    class sky_pipeline final : public graphics_pipeline {
    public:
        explicit sky_pipeline();
        ~sky_pipeline() override;

        struct gpu_vertex_push_constants final {
            DirectX::XMFLOAT4X4A model_view_proj;
        };
        static_assert(sizeof(gpu_vertex_push_constants) <= 128);

        auto render(vk::CommandBuffer cmd) const -> void;

    protected:
        virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void override;

    private:
        eastl::optional<texture> m_skybox_texture {};
        eastl::optional<mesh> m_skydome {};
        vk::DescriptorPool m_descriptor_pool {};
        vk::DescriptorSet m_descriptor_set {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
    };
}
