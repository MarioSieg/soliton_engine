// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_pipeline.hpp"

#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"
#include "mesh.hpp"
#include "material.hpp"

namespace soliton::graphics {
    auto graphics_pipeline::create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> bool {
        const vk::Device device = vkb::ctx().get_device();

        vk::GraphicsPipelineCreateInfo pipeline_info {};

        eastl::vector<vk::PipelineShaderStageCreateInfo> shader_stages {};
        eastl::vector<eastl::shared_ptr<shader>> shaders {};
        if (!configure_shaders(shaders)) {
            log_error("Failed to configure shaders for pipeline '{}'", name);
            return false;
        }
        panic_assert(!shaders.empty());
        shader_stages.reserve(shaders.size());
        for (auto&& shader : shaders) {
            panic_assert(shader != nullptr);
            shader_stages.emplace_back(shader->get_stage_info());
        }
        pipeline_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();

        vk::PipelineViewportStateCreateInfo viewport_state {};
        if (!configure_viewport_state(viewport_state)) {
            log_error("Failed to configure viewport state for pipeline '{}'", name);
            return false;
        }
        pipeline_info.pViewportState = &viewport_state;

        vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
        eastl::vector<vk::VertexInputBindingDescription> vertex_bindings {};
        eastl::vector<vk::VertexInputAttributeDescription> vertex_attributes {};
        if (!configure_vertex_info(vertex_bindings, vertex_attributes)) {
            log_error("Failed to configure vertex input info for pipeline '{}'", name);
            return false;
        }
        vertex_input_info.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_bindings.size());
        vertex_input_info.pVertexBindingDescriptions = vertex_bindings.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_attributes.size());
        vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();
        pipeline_info.pVertexInputState = &vertex_input_info;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        if (!configure_input_assembly(input_assembly_state)) {
            log_error("Failed to configure input assembly state for pipeline '{}'", name);
            return false;
        }
        pipeline_info.pInputAssemblyState = &input_assembly_state;

        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        if (!configure_rasterizer(rasterization_state)) {
            log_error("Failed to configure rasterization state for pipeline '{}'", name);
            return false;
        }
        pipeline_info.pRasterizationState = &rasterization_state;

        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        eastl::vector<vk::DynamicState> dynamic_states {};
        if (!configure_dynamic_states(dynamic_states)) {
            log_error("Failed to configure dynamic states for pipeline '{}'", name);
            return false;
        }
        dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();
        pipeline_info.pDynamicState = &dynamic_state;

        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        if (!configure_multisampling(multisample_state)) {
            log_error("Failed to configure multisampling for pipeline '{}'", name);
            return false;
        }
        pipeline_info.pMultisampleState = &multisample_state;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {};
        if (!configure_depth_stencil(depth_stencil_state)) {
            log_error("Failed to configure depth stencil state for pipeline '{}'", name);
            return false;
        }
        pipeline_info.pDepthStencilState = &depth_stencil_state;

        vk::PipelineColorBlendAttachmentState blend_attachment_state {};
        if (!configure_color_blending(blend_attachment_state)) {
            log_error("Failed to configure color blending for pipeline '{}'", name);
            return false;
        }

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.logicOpEnable = vk::False;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;
        pipeline_info.pColorBlendState = &color_blend_state;

        vk::RenderPass render_pass {};
        if (!configure_render_pass(render_pass)) {
            log_error("Failed to configure render pass for pipeline '{}'", name);
            return false;
        }
        panic_assert(render_pass);
        pipeline_info.renderPass = render_pass;

        // finally, create pipeline layout
        eastl::vector<vk::DescriptorSetLayout> layouts {};
        eastl::vector<vk::PushConstantRange> ranges {};
        if (!configure_pipeline_layout(layouts, ranges)) {
            log_error("Failed to configure pipeline layout for pipeline '{}'", name);
            return false;
        }
        vk::PipelineLayoutCreateInfo layout_info {};
        layout_info.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        layout_info.pSetLayouts = layouts.data();
        layout_info.pushConstantRangeCount = static_cast<std::uint32_t>(ranges.size());
        layout_info.pPushConstantRanges = ranges.data();
        vkcheck(device.createPipelineLayout(&layout_info, vkb::get_alloc(), &out_layout));
        pipeline_info.layout = out_layout;

        // create pipeline
        vkcheck(device.createGraphicsPipelines(cache, 1, &pipeline_info, vkb::get_alloc(), &out_pipeline));
        return true;
    }

    auto graphics_pipeline::configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.viewportCount = 1;
        cfg.scissorCount = 1;
        cfg.pViewports = nullptr;
        return true;
    }

    auto graphics_pipeline::configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.topology = vk::PrimitiveTopology::eTriangleList;
        cfg.primitiveRestartEnable = vk::False;
        return true;
    }

    auto graphics_pipeline::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.polygonMode = vk::PolygonMode::eFill;
        cfg.cullMode = vk::CullModeFlagBits::eBack;
        cfg.frontFace = vk::FrontFace::eClockwise;
        cfg.depthClampEnable = vk::False;
        cfg.rasterizerDiscardEnable = vk::False;
        cfg.depthBiasEnable = vk::False;
        cfg.lineWidth = 1.0f;
        return true;
    }

    auto graphics_pipeline::configure_dynamic_states(eastl::vector<vk::DynamicState>& states) -> bool {
        panic_assert(type == pipeline_type::graphics);
        states.emplace_back(vk::DynamicState::eViewport);
        states.emplace_back(vk::DynamicState::eScissor);
        return true;
    }

    auto graphics_pipeline::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.depthTestEnable = vk::True;
        cfg.depthWriteEnable = vk::True;
        cfg.depthCompareOp = vk::CompareOp::eLessOrEqual;
        cfg.depthBoundsTestEnable = vk::False;
        cfg.back.failOp = vk::StencilOp::eKeep;
        cfg.back.passOp = vk::StencilOp::eKeep;
        cfg.back.compareOp = vk::CompareOp::eAlways;
        cfg.stencilTestEnable = vk::False;
        cfg.front = cfg.back;
        return true;
    }

    auto graphics_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::ctx().get_msaa_samples();
        cfg.alphaToCoverageEnable = vk::False;
        return true;
    }

    auto graphics_pipeline::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        cfg.blendEnable = vk::False;
        cfg.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        cfg.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        cfg.colorBlendOp = vk::BlendOp::eAdd;
        cfg.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        cfg.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        cfg.alphaBlendOp = vk::BlendOp::eAdd;
        return true;
    }

    auto graphics_pipeline::configure_render_pass(vk::RenderPass& pass) -> bool {
        panic_assert(type == pipeline_type::graphics);
        pass = vkb::ctx().get_scene_render_pass();
        return true;
    }

    auto graphics_pipeline::configure_vertex_info(eastl::vector<vk::VertexInputBindingDescription>& cfg, eastl::vector<vk::VertexInputAttributeDescription>& bindings) -> bool {
        cfg.insert(cfg.end(), k_vertex_binding_desc.begin(), k_vertex_binding_desc.end());
        bindings.insert(bindings.end(), k_vertex_attrib_desc.begin(), k_vertex_attrib_desc.end());
        return true;
    }

    auto graphics_pipeline::configure_enable_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
        panic_assert(type == pipeline_type::graphics);
        cfg.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        cfg.blendEnable = vk::True;
        cfg.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        cfg.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        cfg.colorBlendOp = vk::BlendOp::eAdd;
        cfg.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        cfg.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        cfg.alphaBlendOp = vk::BlendOp::eAdd;
    }

    auto graphics_pipeline::on_bind(vkb::command_buffer& cmd) const -> void {
        cmd.bind_pipeline(*this);
    }
}
