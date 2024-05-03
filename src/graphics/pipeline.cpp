// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline.hpp"

#include "vulkancore/context.hpp"

namespace graphics {
    pipeline_base::~pipeline_base() {
        const vk::Device device = vkb::ctx().get_device();
        if (m_pipeline && m_layout) { // Destroy old pipeline and layout
            device.destroyPipeline(m_pipeline, &vkb::s_allocator);
            device.destroyPipelineLayout(m_layout, &vkb::s_allocator);
            m_pipeline = nullptr;
            m_layout = nullptr;
        }
    }

    auto pipeline_base::create(const vk::PipelineCache cache) -> bool {
        log_info("Creating graphics pipeline '{}' {}st time", name, ++m_num_creations);
        const auto now = std::chrono::high_resolution_clock::now();

        const vk::Device device = vkb::ctx().get_device();

        auto prev_pipeline = m_pipeline;
        auto prev_layout = m_layout;
        auto restore_state = [&] {
            m_pipeline = prev_pipeline;
            m_layout = prev_layout;
        };

        passert(type == pipeline_type::graphics); // TODO Only graphics pipelines are supported

        if (!pre_configure()) [[unlikely]] {
            log_error("Failed to pre-configure pipeline '{}'", name);
            restore_state();
            return false;
        }

        vk::GraphicsPipelineCreateInfo pipeline_info {};

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages {};
        std::vector<std::pair<std::unique_ptr<vkb::shader>, vk::ShaderStageFlagBits>> shaders {};
        if (!configure_shaders(shaders)) [[unlikely]] {
            log_error("Failed to configure shaders for pipeline '{}'", name);
            restore_state();
            return false;
        }
        passert(!shaders.empty());
        shader_stages.reserve(shaders.size());
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
        if (!configure_viewport_state(viewport_state)) [[unlikely]] {
            log_error("Failed to configure viewport state for pipeline '{}'", name);
            restore_state();
            return false;
        }
        pipeline_info.pViewportState = &viewport_state;

        vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
        std::vector<vk::VertexInputBindingDescription> vertex_bindings {};
        std::vector<vk::VertexInputAttributeDescription> vertex_attributes {};
        if (!configure_vertex_info(vertex_bindings, vertex_attributes)) [[unlikely]] {
            log_error("Failed to configure vertex info for pipeline '{}'", name);
            restore_state();
            return false;
        }
        vertex_input_info.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_bindings.size());
        vertex_input_info.pVertexBindingDescriptions = vertex_bindings.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_attributes.size());
        vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();
        pipeline_info.pVertexInputState = &vertex_input_info;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        if (!configure_input_assembly(input_assembly_state)) [[unlikely]] {
            log_error("Failed to configure input assembly state for pipeline '{}'", name);
            restore_state();
            return false;
        }
        pipeline_info.pInputAssemblyState = &input_assembly_state;

        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        if (!configure_rasterizer(rasterization_state)) [[unlikely]] {
            log_error("Failed to configure rasterization state for pipeline '{}'", name);
            restore_state();
            return false;
        }
        pipeline_info.pRasterizationState = &rasterization_state;

        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        std::vector<vk::DynamicState> dynamic_states {};
        if (!configure_dynamic_states(dynamic_states)) [[unlikely]] {
            log_error("Failed to configure dynamic states for pipeline '{}'", name);
            restore_state();
            return false;
        }
        dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();
        pipeline_info.pDynamicState = &dynamic_state;

        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        if (!configure_multisampling(multisample_state)) [[unlikely]] {
            log_error("Failed to configure multisampling for pipeline '{}'", name);
            restore_state();
            return false;
        }
        pipeline_info.pMultisampleState = &multisample_state;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {};
        if (!configure_depth_stencil(depth_stencil_state)) [[unlikely]] {
            log_error("Failed to configure depth stencil state for pipeline '{}'", name);
            restore_state();
            return false;
        }
        pipeline_info.pDepthStencilState = &depth_stencil_state;

        vk::PipelineColorBlendAttachmentState blend_attachment_state {};
        if (!configure_color_blending(blend_attachment_state)) [[unlikely]] {
            log_error("Failed to configure color blending for pipeline '{}'", name);
            restore_state();
            return false;
        }

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.logicOpEnable = vk::False;
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;
        pipeline_info.pColorBlendState = &color_blend_state;

        vk::RenderPass render_pass {};
        if (!configure_render_pass(render_pass)) [[unlikely]] {
            log_error("Failed to configure render pass for pipeline '{}'", name);
            restore_state();
            return false;
        }
        passert(render_pass);
        pipeline_info.renderPass = render_pass;

        // finally, create pipeline layout
        std::vector<vk::DescriptorSetLayout> layouts {};
        std::vector<vk::PushConstantRange> ranges {};
        if (!configure_pipeline_layout(layouts, ranges)) [[unlikely]] {
            log_error("Failed to configure pipeline layout for pipeline '{}'", name);
            restore_state();
            return false;
        }
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

        if (!post_configure()) [[unlikely]] {
            log_error("Failed to post-configure pipeline '{}'", name);
            restore_state();
            return false;
        }

        if (prev_pipeline && prev_layout) { // Destroy old pipeline and layout now that the new one is created
            device.destroyPipeline(prev_pipeline, &vkb::s_allocator);
            device.destroyPipelineLayout(prev_layout, &vkb::s_allocator);
        }

        return true;
    }

    pipeline_base::pipeline_base(std::string&& name, const pipeline_type type) : name{std::move(name)}, type{type} {
    }

    auto pipeline_base::pre_configure() -> bool {
        return true;
    }

    auto pipeline_base::configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> bool {
        passert(type == pipeline_type::graphics);
        cfg.viewportCount = 1;
        cfg.scissorCount = 1;
        cfg.pViewports = nullptr;
        return true;
    }

    auto pipeline_base::configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> bool {
        passert(type == pipeline_type::graphics);
        cfg.topology = vk::PrimitiveTopology::eTriangleList;
        cfg.primitiveRestartEnable = vk::False;
        return true;
    }

    auto pipeline_base::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool {
        passert(type == pipeline_type::graphics);
        cfg.polygonMode = vk::PolygonMode::eFill;
        cfg.cullMode = vk::CullModeFlagBits::eBack;
        cfg.frontFace = vk::FrontFace::eClockwise;
        cfg.depthClampEnable = vk::False;
        cfg.rasterizerDiscardEnable = vk::False;
        cfg.depthBiasEnable = vk::False;
        cfg.lineWidth = 1.0f;
        return true;
    }

    auto pipeline_base::configure_dynamic_states(std::vector<vk::DynamicState>& states) -> bool {
        passert(type == pipeline_type::graphics);
        states.emplace_back(vk::DynamicState::eViewport);
        states.emplace_back(vk::DynamicState::eScissor);
        return true;
    }

    auto pipeline_base::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> bool {
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
        return true;
    }

    auto pipeline_base::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> bool {
        passert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::k_msaa_sample_count;
        cfg.alphaToCoverageEnable = vk::False;
        cfg.pSampleMask = nullptr;
        return true;
    }

    auto pipeline_base::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> bool {
        passert(type == pipeline_type::graphics);
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

    auto pipeline_base::configure_render_pass(vk::RenderPass& pass) -> bool {
        passert(type == pipeline_type::graphics);
        pass = vkb::ctx().get_scene_render_pass();
        return true;
    }

    auto pipeline_base::post_configure() -> bool {
        return true;
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

    static inline constinit std::atomic_bool s_init;

    auto pipeline_registry::init() -> void {
        if (s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance = std::make_unique<pipeline_registry>(vkb::vkdvc());
        s_init.store(true, std::memory_order_relaxed);
    }

    auto pipeline_registry::shutdown() -> void {
        if (!s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance.reset();
        s_init.store(false, std::memory_order_relaxed);
    }

    auto pipeline_registry::invalidate_all() -> void {
        m_pipelines.clear();
        m_names.clear();
    }

    auto pipeline_registry::try_recreate_all() -> bool {
        bool success = true;
        const auto now = std::chrono::high_resolution_clock::now();
        std::size_t i = 0;
        for (const auto& [name, pipeline] : m_pipelines) {
            if (!pipeline->create(m_cache)) {
                log_error("Failed to recreate pipeline '{}'", name);
                success = false;
                break;
            }
            ++i;
        }
        if (success) [[likely]] {
            log_info("Recreated {} pipelines in {:.03}s", i, std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - now).count());
        }
        return success;
    }
}
