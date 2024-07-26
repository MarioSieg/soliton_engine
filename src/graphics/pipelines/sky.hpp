// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../pipeline.hpp"
#include "../texture.hpp"
#include "../mesh.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    class sky_pipeline final : public pipeline_base {
    public:
        explicit sky_pipeline();
        ~sky_pipeline() override;

        struct gpu_vertex_push_constants final {
            DirectX::XMFLOAT4X4A model_view_proj;
        };
        static_assert(sizeof(gpu_vertex_push_constants) <= 128);

        auto render(vk::CommandBuffer cmd) const -> void;

    protected:
        virtual auto configure_shaders(std::vector<std::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void override;

    private:
        std::optional<texture> m_skybox_texture {};
        std::optional<mesh> m_skydome {};
        vk::DescriptorPool m_descriptor_pool {};
        vk::DescriptorSet m_descriptor_set {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
    };
}
