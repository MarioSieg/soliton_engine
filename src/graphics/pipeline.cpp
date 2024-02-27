// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline.hpp"

#include "vulkancore/context.hpp"

namespace graphics {
    pipeline_base::~pipeline_base() {
        const vk::Device device = vkb_context().get_device();
        if (m_pipeline && m_layout) { // Destroy old pipeline and layout
            device.destroyPipeline(m_pipeline, &vkb::s_allocator);
            device.destroyPipelineLayout(m_layout, &vkb::s_allocator);
            m_pipeline = nullptr;
            m_layout = nullptr;
        }
    }

    auto pipeline_base::create(const vk::PipelineCache cache) -> void {
        log_info("Creating graphics pipeline '{}' {}st time", name, ++m_num_creations);
        const auto now = std::chrono::high_resolution_clock::now();

        const vk::Device device = vkb_context().get_device();

        if (m_pipeline && m_layout) { // Destroy old pipeline and layout
            device.destroyPipeline(m_pipeline, &vkb::s_allocator);
            device.destroyPipelineLayout(m_layout, &vkb::s_allocator);
            m_pipeline = nullptr;
            m_layout = nullptr;
        }

        passert(type == pipeline_type::graphics); // TODO Only graphics pipelines are supported

        pre_configure();

        vk::GraphicsPipelineCreateInfo pipeline_info {};

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages {};
        std::vector<std::pair<std::unique_ptr<vkb::shader>, vk::ShaderStageFlagBits>> shaders {};
        configure_shaders(shaders);
        passert(!shaders.empty());
        for (const auto& [shader, stage] : shaders) {
            shader_stages.emplace_back(vk::PipelineShaderStageCreateInfo {
                .stage = stage,
                .module = shader->get_module(),
                .pName = "main"
            });
        }
        pipeline_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();

        vk::PipelineViewportStateCreateInfo viewport_state {};
        configure_viewport_state(viewport_state);
        pipeline_info.pViewportState = &viewport_state;

        vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
        std::vector<vk::VertexInputBindingDescription> vertex_bindings {};
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes {};
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
        std::vector<vk::DynamicState> dynamic_states {};
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
        std::vector<vk::DescriptorSetLayout> layouts {};
        std::vector<vk::PushConstantRange> ranges {};
        configure_pipeline_layout(layouts, ranges);
        vk::PipelineLayoutCreateInfo layout_info {};
        layout_info.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
        layout_info.pSetLayouts = layouts.data();
        layout_info.pushConstantRangeCount = static_cast<std::uint32_t>(ranges.size());
        layout_info.pPushConstantRanges = ranges.data();
        vkcheck(device.createPipelineLayout(&layout_info, &vkb::s_allocator, &m_layout));
        pipeline_info.layout = m_layout;

        // create pipeline
        vkcheck(device.createGraphicsPipelines(cache, 1, &pipeline_info, &vkb::s_allocator, &m_pipeline));

        log_info("Created graphics pipeline '{}' in {}s", name, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - now).count());

        post_configure();
    }

    pipeline_base::pipeline_base(std::string&& name, const pipeline_type type) : name{std::move(name)}, type{type} {
    }

    auto pipeline_base::pre_configure() -> void {

    }

    auto pipeline_base::configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.viewportCount = 1;
        cfg.scissorCount = 1;
        cfg.pViewports = nullptr;
    }

    auto pipeline_base::configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.topology = vk::PrimitiveTopology::eTriangleList;
        cfg.primitiveRestartEnable = vk::False;
    }

    auto pipeline_base::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.polygonMode = vk::PolygonMode::eFill;
        cfg.cullMode = vk::CullModeFlagBits::eBack;
        cfg.frontFace = vk::FrontFace::eClockwise;
        cfg.depthClampEnable = vk::False;
        cfg.rasterizerDiscardEnable = vk::False;
        cfg.depthBiasEnable = vk::False;
        cfg.lineWidth = 1.0f;
    }

    auto pipeline_base::configure_dynamic_states(std::vector<vk::DynamicState>& states) -> void {
        passert(type == pipeline_type::graphics);
        states.emplace_back(vk::DynamicState::eViewport);
        states.emplace_back(vk::DynamicState::eScissor);
    }

    auto pipeline_base::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void {
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

    auto pipeline_base::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::k_msaa_sample_count;
        cfg.alphaToCoverageEnable = vk::False;
        cfg.pSampleMask = nullptr;
    }

    auto pipeline_base::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
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

    auto pipeline_base::configure_render_pass(vk::RenderPass& pass) -> void {
        passert(type == pipeline_type::graphics);
        pass = vkb_context().get_render_pass();
    }

    auto pipeline_base::post_configure() -> void {

    }

    pipeline_registry::pipeline_registry(const vk::Device device) : m_device{device} {
        passert(device);
        vk::PipelineCacheCreateInfo cache_info {};
        vkcheck(device.createPipelineCache(&cache_info, &vkb::s_allocator, &m_cache));
    }

    pipeline_registry::~pipeline_registry() {
        m_pipelines.clear();
        if (m_cache) {
            m_device.destroyPipelineCache(m_cache, &vkb::s_allocator);
            m_cache = nullptr;
        }
    }
}
