// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXMath.h>

#include "../pipeline.hpp"
#include "../../scene/components.hpp"

namespace graphics::pipelines {
    class pbr_pipeline final : public pipeline_base {
    public:
        explicit pbr_pipeline();
        ~pbr_pipeline() override;

        struct gpu_vertex_push_constants final {
            DirectX::XMFLOAT4X4A model_view_proj;
            DirectX::XMFLOAT4X4A normal_matrix;
        };
        static_assert(sizeof(gpu_vertex_push_constants) <= 128);

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
        virtual auto configure_shaders(std::vector<std::pair<std::shared_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> void;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void;

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
