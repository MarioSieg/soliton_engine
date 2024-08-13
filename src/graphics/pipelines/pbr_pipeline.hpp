// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXMath.h>

#include "../vulkancore/command_buffer.hpp"
#include "../graphics_pipeline.hpp"
#include "../pbr_filter_processor.hpp"
#include "../../scene/components.hpp"

namespace lu::graphics::pipelines {
    class pbr_pipeline final : public graphics_pipeline {
    public:
        explicit pbr_pipeline();
        ~pbr_pipeline() override;

        struct push_constants_vs final {
            DirectX::XMFLOAT4X4A model_matrix;
            DirectX::XMFLOAT4X4A model_view_proj;
            DirectX::XMFLOAT4X4A normal_matrix;
        };

        struct push_constants_fs final {
            DirectX::XMFLOAT4A data; // xyz: camera position, w: time
        };

    private:
        pbr_filter_processor m_pbr_filter_processor {};
        vk::DescriptorSetLayout m_pbr_descriptor_set_layout {};
        vk::DescriptorSet m_pbr_descriptor_set {};

        virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void override;
        virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void override;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void override;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void override;

        HOTPROC virtual auto render_single_mesh(
            vkb::command_buffer& cmd,
            const mesh& mesh,
            const com::mesh_renderer& renderer,
            DirectX::FXMMATRIX view_proj_mtx,
            DirectX::CXMMATRIX model_mtx
        ) const noexcept -> void final override;

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void override;
    };
}
