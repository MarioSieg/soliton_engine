// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXMath.h>

#include "../vulkancore/command_buffer.hpp"
#include "../graphics_pipeline.hpp"
#include "../pbr_filter_processor.hpp"
#include "../temporal_blue_noise.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    class pbr_pipeline final : public graphics_pipeline {
    public:
        explicit pbr_pipeline();
        ~pbr_pipeline() override;

        struct push_constants_vs final {
            XMFLOAT4X4A model_matrix;
            XMFLOAT4X4A model_view_proj;
            XMFLOAT4X4A normal_matrix;
        };

        HOTPROC auto render_mesh_renderer(
            vkb::command_buffer& cmd,
            const com::transform& transform,
            const com::mesh_renderer& renderer,
            const BoundingFrustum& frustum,
            FXMMATRIX view_proj_mtx,
            CXMMATRIX view_mtx
        ) const noexcept -> void;

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void override;

    private:
        pbr_filter_processor m_pbr_filter_processor {};
        vk::DescriptorSetLayout m_pbr_descriptor_set_layout {};
        vk::DescriptorSet m_pbr_descriptor_set {};

        virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void override;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void override;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void override;

        HOTPROC auto render_single_mesh(
            vkb::command_buffer& cmd,
            const mesh& mesh,
            const com::mesh_renderer& renderer,
            FXMMATRIX view_proj_mtx,
            CXMMATRIX model_mtx,
            CXMMATRIX view_mtx
        ) const noexcept -> void;
    };
}
