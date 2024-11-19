// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "pipeline_base.hpp"

#include "../scene/components.hpp"

namespace soliton::graphics {
    class graphics_pipeline : public pipeline_base {
    public:
        virtual ~graphics_pipeline() override = default;

    protected:
        explicit graphics_pipeline(eastl::string&& name) : pipeline_base{std::move(name), pipeline_type::graphics} {}

         [[nodiscard]] virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> bool = 0;
         [[nodiscard]] virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> bool = 0;
         [[nodiscard]] virtual auto configure_vertex_info(eastl::vector<vk::VertexInputBindingDescription>& cfg, eastl::vector<vk::VertexInputAttributeDescription>& bindings) -> bool;
         [[nodiscard]] virtual auto configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> bool;
         [[nodiscard]] virtual auto configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> bool;
         [[nodiscard]] virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool;
         [[nodiscard]] virtual auto configure_dynamic_states(eastl::vector<vk::DynamicState>& states) -> bool;
         [[nodiscard]] virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> bool;
         [[nodiscard]] virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> bool;
         [[nodiscard]] virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> bool;
         [[nodiscard]] virtual auto configure_render_pass(vk::RenderPass& pass) -> bool;

        auto configure_enable_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void;

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void;

    private:
        friend class graphics_subsystem;

        [[nodiscard]] auto create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> bool override final;
    };
}
