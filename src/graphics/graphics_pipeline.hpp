// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "pipeline_base.hpp"

namespace lu::graphics {
    class graphics_pipeline : public pipeline_base {
    public:
        virtual ~graphics_pipeline() override = default;

    protected:
        explicit graphics_pipeline(eastl::string&& name) : pipeline_base{std::move(name), pipeline_type::graphics} {}

        HOTPROC static auto draw_mesh(
            const mesh& mesh,
            vk::CommandBuffer cmd,
            const eastl::vector<material*>& mats,
            vk::PipelineLayout layout
        ) -> void;

        HOTPROC static auto draw_mesh(
            const mesh& mesh,
            vk::CommandBuffer cmd
        ) -> void;

        virtual auto configure_shaders(eastl::vector<std::shared_ptr<shader>>& cfg) -> void = 0;
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

    private:
        auto create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> void override final;
    };
}
