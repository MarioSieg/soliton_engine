// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_pipeline.hpp"

#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"
#include "mesh.hpp"
#include "material.hpp"

namespace lu::graphics {
    auto graphics_pipeline::create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> void {
        const vk::Device device = vkb::ctx().get_device();

        vk::GraphicsPipelineCreateInfo pipeline_info {};

        eastl::vector<vk::PipelineShaderStageCreateInfo> shader_stages {};
        eastl::vector<eastl::shared_ptr<shader>> shaders {};
        configure_shaders(shaders);
        passert(!shaders.empty());
        shader_stages.reserve(shaders.size());
        for (auto&& shader : shaders) {
            shader_stages.emplace_back(shader->get_stage_info());
        }
        pipeline_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();

        vk::PipelineViewportStateCreateInfo viewport_state {};
        configure_viewport_state(viewport_state);
        pipeline_info.pViewportState = &viewport_state;

        vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
        eastl::vector<vk::VertexInputBindingDescription> vertex_bindings {};
        eastl::vector<vk::VertexInputAttributeDescription> vertex_attributes {};
        configure_vertex_info(vertex_bindings, vertex_attributes);
        vertex_input_info.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_bindings.size());
        vertex_input_info.pVertexBindingDescriptions = vertex_bindings.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_attributes.size());
        vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();
        pipeline_info.pVertexInputState = &vertex_input_info;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        configure_input_assembly(input_assembly_state);
        pipeline_info.pInputAssemblyState = &input_assembly_state;

        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        configure_rasterizer(rasterization_state);
        pipeline_info.pRasterizationState = &rasterization_state;

        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        eastl::vector<vk::DynamicState> dynamic_states {};
        configure_dynamic_states(dynamic_states);
        dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();
        pipeline_info.pDynamicState = &dynamic_state;

        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        configure_multisampling(multisample_state);
        pipeline_info.pMultisampleState = &multisample_state;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {};
        configure_depth_stencil(depth_stencil_state);
        pipeline_info.pDepthStencilState = &depth_stencil_state;

        vk::PipelineColorBlendAttachmentState blend_attachment_state {};
        configure_color_blending(blend_attachment_state);

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.logicOpEnable = vk::False;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;
        pipeline_info.pColorBlendState = &color_blend_state;

        vk::RenderPass render_pass {};
        configure_render_pass(render_pass);
        passert(render_pass);
        pipeline_info.renderPass = render_pass;

        // finally, create pipeline layout
        eastl::vector<vk::DescriptorSetLayout> layouts {};
        eastl::vector<vk::PushConstantRange> ranges {};
        configure_pipeline_layout(layouts, ranges);
        vk::PipelineLayoutCreateInfo layout_info {};
        layout_info.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        layout_info.pSetLayouts = layouts.data();
        layout_info.pushConstantRangeCount = static_cast<std::uint32_t>(ranges.size());
        layout_info.pPushConstantRanges = ranges.data();
        vkcheck(device.createPipelineLayout(&layout_info, vkb::get_alloc(), &out_layout));
        pipeline_info.layout = out_layout;

        // create pipeline
        vkcheck(device.createGraphicsPipelines(cache, 1, &pipeline_info, vkb::get_alloc(), &out_pipeline));
    }

    auto graphics_pipeline::configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.viewportCount = 1;
        cfg.scissorCount = 1;
        cfg.pViewports = nullptr;
    }

    auto graphics_pipeline::configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.topology = vk::PrimitiveTopology::eTriangleList;
        cfg.primitiveRestartEnable = vk::False;
    }

    auto graphics_pipeline::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.polygonMode = vk::PolygonMode::eFill;
        cfg.cullMode = vk::CullModeFlagBits::eBack;
        cfg.frontFace = vk::FrontFace::eClockwise;
        cfg.depthClampEnable = vk::False;
        cfg.rasterizerDiscardEnable = vk::False;
        cfg.depthBiasEnable = vk::False;
        cfg.lineWidth = 1.0f;
    }

    auto graphics_pipeline::configure_dynamic_states(eastl::vector<vk::DynamicState>& states) -> void {
        passert(type == pipeline_type::graphics);
        states.emplace_back(vk::DynamicState::eViewport);
        states.emplace_back(vk::DynamicState::eScissor);
    }

    auto graphics_pipeline::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.depthTestEnable = vk::True;
        cfg.depthWriteEnable = vk::True;
        cfg.depthCompareOp = vk::CompareOp::eLessOrEqual;
        cfg.depthBoundsTestEnable = vk::False;
        cfg.back.failOp = vk::StencilOp::eKeep;
        cfg.back.passOp = vk::StencilOp::eKeep;
        cfg.back.compareOp = vk::CompareOp::eAlways;
        cfg.stencilTestEnable = vk::False;
        cfg.front = cfg.back;
    }

    auto graphics_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::k_msaa_sample_count;
        cfg.alphaToCoverageEnable = vk::False;
    }

    auto graphics_pipeline::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        cfg.blendEnable = vk::False;
        cfg.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        cfg.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        cfg.colorBlendOp = vk::BlendOp::eAdd;
        cfg.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        cfg.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        cfg.alphaBlendOp = vk::BlendOp::eAdd;
    }

    auto graphics_pipeline::configure_render_pass(vk::RenderPass& pass) -> void {
        passert(type == pipeline_type::graphics);
        pass = vkb::ctx().get_scene_render_pass();
    }

    auto graphics_pipeline::configure_vertex_info(eastl::vector<vk::VertexInputBindingDescription>& cfg, eastl::vector<vk::VertexInputAttributeDescription>& bindings) -> void {
        cfg.emplace_back(vk::VertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(mesh::vertex),
                .inputRate = vk::VertexInputRate::eVertex
        });

        auto push_attribute = [&](const vk::Format format, const std::uint32_t offset) mutable  {
            vk::VertexInputAttributeDescription& desc = bindings.emplace_back();
            desc.location = bindings.size() - 1;
            desc.binding = 0;
            desc.format = format;
            desc.offset = offset;
        };
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, position));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, normal));
        push_attribute(vk::Format::eR32G32Sfloat, offsetof(mesh::vertex, uv));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, tangent));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, bitangent));
    }

    // graphics_pipeline! MIGHT BE RENDER THREAD LOCAL
    auto graphics_pipeline::draw_mesh(const mesh& mesh, const vk::CommandBuffer cmd) -> void {
        constexpr vk::DeviceSize offsets = 0;
        cmd.bindIndexBuffer(mesh.get_index_buffer().get_buffer(), 0, mesh.is_index_32bit() ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
        cmd.bindVertexBuffers(0, 1, &mesh.get_vertex_buffer().get_buffer(), &offsets);
        for (const mesh::primitive& prim : mesh.get_primitives()) {
            cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
        }
    }

    // WARNING! MIGHT BE RENDER THREAD LOCAL
    auto graphics_pipeline::draw_mesh(
        const mesh& mesh,
        const vk::CommandBuffer cmd,
        const eastl::span<material* const> mats,
        const vk::PipelineLayout layout
    ) -> void {
        constexpr vk::DeviceSize offsets = 0;
        cmd.bindIndexBuffer(mesh.get_index_buffer().get_buffer(), 0, mesh.is_index_32bit() ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
        cmd.bindVertexBuffers(0, 1, &mesh.get_vertex_buffer().get_buffer(), &offsets);
        if (mesh.get_primitives().size() <= mats.size()) { // we have at least one material for each primitive
            for (std::size_t idx = 0; const mesh::primitive& prim : mesh.get_primitives()) {
                cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    layout,
                    0,
                    1,
                    &mats[idx++]->get_descriptor_set(),
                    0,
                    nullptr
                );
                cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
                graphics_subsystem::s_num_draw_calls.fetch_add(1, std::memory_order_relaxed);
                graphics_subsystem::s_num_draw_verts.fetch_add(prim.vertex_count, std::memory_order_relaxed);
            }
        } else {
            cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    layout,
                    0,
                    1,
                    &mats[0]->get_descriptor_set(),
                    0,
                    nullptr
            );
            cmd.drawIndexed(mesh.get_index_count(), 1, 0, 0, 0);
            graphics_subsystem::s_num_draw_calls.fetch_add(1, std::memory_order_relaxed);
            graphics_subsystem::s_num_draw_verts.fetch_add(mesh.get_vertex_count(), std::memory_order_relaxed);
        }
    }

    auto graphics_pipeline::configure_enable_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        cfg.blendEnable = vk::True;
        cfg.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        cfg.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        cfg.colorBlendOp = vk::BlendOp::eAdd;
        cfg.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        cfg.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        cfg.alphaBlendOp = vk::BlendOp::eAdd;
    }
}
