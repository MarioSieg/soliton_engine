// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXMath.h>

#include "../graphics_pipeline.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    class pbr_pipeline final : public graphics_pipeline {
    public:
        explicit pbr_pipeline();
        ~pbr_pipeline() override;

        struct push_constants_vs final {
            DirectX::XMFLOAT4X4A model_view_proj;
            DirectX::XMFLOAT4X4A normal_matrix;
        };
        static_assert(sizeof(push_constants_vs) <= 128);

        struct push_constants_fs final {
            float time;
        };
        static_assert(sizeof(push_constants_fs) <= 128);

        // WARNING! RENDER THREAD LOCAL
        HOTPROC auto XM_CALLCONV render_mesh(
            vk::CommandBuffer cmd_buf,
            vk::PipelineLayout layout,
            const com::transform& transform,
            const com::mesh_renderer& renderer,
            const DirectX::BoundingFrustum& frustum,
            DirectX::FXMMATRIX vp
        ) const -> void;

    protected:
        virtual auto configure_shaders(std::vector<std::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void override;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void override;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void override;

    private:
        // Generate a BRDF integration map used as a look-up-table (stores roughness / NdotV)
        auto generate_brdf_lut() -> void;

        struct {
            vk::Image image {};
            vk::ImageView m_image_view {};
            vk::DeviceMemory memory {};
        } m_brdf_lut {};
    };
}
