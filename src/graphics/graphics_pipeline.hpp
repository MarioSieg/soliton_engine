// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "pipeline_base.hpp"

#include "../scene/components.hpp"

namespace lu::graphics {
    class graphics_pipeline : public pipeline_base {
    public:
        virtual ~graphics_pipeline() override = default;

    protected:
        explicit graphics_pipeline(eastl::string&& name) : pipeline_base{std::move(name), pipeline_type::graphics} {}

        virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void = 0;
        virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void = 0;
        virtual auto configure_vertex_info(eastl::vector<vk::VertexInputBindingDescription>& cfg, eastl::vector<vk::VertexInputAttributeDescription>& bindings) -> void;
        virtual auto configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> void;
        virtual auto configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> void;
        virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void;
        virtual auto configure_dynamic_states(eastl::vector<vk::DynamicState>& states) -> void;
        virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void;
        virtual auto configure_render_pass(vk::RenderPass& pass) -> void;
        auto configure_enable_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void;

        HOTPROC virtual auto render_single_mesh(
            vkb::command_buffer& cmd,
            const mesh& mesh,
            const com::mesh_renderer& renderer,
            DirectX::FXMMATRIX view_proj_mtx,
            DirectX::CXMMATRIX model_mtx,
            DirectX::CXMMATRIX view_mtx
        ) const noexcept -> void = 0;
        virtual auto on_bind(vkb::command_buffer& cmd) const -> void;

    private:
        friend class graphics_subsystem;

        HOTPROC auto render_mesh(
            vkb::command_buffer& cmd,
            const com::transform& transform,
            const com::mesh_renderer& renderer,
            const DirectX::BoundingFrustum& frustum,
            DirectX::FXMMATRIX view_proj_mtx,
            DirectX::CXMMATRIX view_mtx
        ) const noexcept -> void;

        auto create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> void override final;
    };
}
