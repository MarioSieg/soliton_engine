// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../vulkancore/context.hpp"
#include "../../scripting/convar.hpp"

namespace lu::graphics::pipelines {
    static constexpr vk::Format brfd_lut_fmt = vk::Format::eR16G16Sfloat;
    static convar<std::uint32_t> brdf_lut_size { "Renderer.brdfLutSize", {{512u}}, convar_flags::read_only, 128u, 8192u };

    auto pbr_pipeline::generate_brdf_lut() -> void {
        log_info("Generating BRDF LUT...");
        const auto now = std::chrono::high_resolution_clock::now();

        const vk::Device dvc = vkb::vkdvc();

        vk::Sampler sampler {};

        vk::ImageCreateInfo image_ci {};
        image_ci.imageType = vk::ImageType::e2D;
        image_ci.format = brfd_lut_fmt;
        image_ci.extent = { brdf_lut_size(), brdf_lut_size(), 1 };
        image_ci.mipLevels = 1;
        image_ci.arrayLayers = 1;
        image_ci.samples = vk::SampleCountFlagBits::e1;
        image_ci.tiling = vk::ImageTiling::eOptimal;
        image_ci.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment;

        vkcheck(dvc.createImage(&image_ci, vkb::get_alloc(), &m_brdf_lut.image));

        vk::MemoryAllocateInfo mem_alloc {}; // TODO: VMA
        vk::MemoryRequirements mem_reqs {};
        dvc.getImageMemoryRequirements(m_brdf_lut.image, &mem_reqs);
        mem_alloc.allocationSize = mem_reqs.size;
        vk::Bool32 found = vk::False;
        mem_alloc.memoryTypeIndex = vkb::ctx().get_device().get_mem_type_idx(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, found);
        passert(found == vk::True);
        vkcheck(dvc.allocateMemory(&mem_alloc, vkb::get_alloc(), &m_brdf_lut.memory));
        vkcheck(dvc.bindImageMemory(m_brdf_lut.image, m_brdf_lut.memory, 0));

        vk::ImageViewCreateInfo view_ci {};
        view_ci.image = m_brdf_lut.image;
        view_ci.viewType = vk::ImageViewType::e2D;
        view_ci.format = brfd_lut_fmt;
        view_ci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        view_ci.subresourceRange.baseMipLevel = 0;
        view_ci.subresourceRange.levelCount = 1;
        view_ci.subresourceRange.baseArrayLayer = 0;
        view_ci.subresourceRange.layerCount = 1;
        vkcheck(dvc.createImageView(&view_ci, vkb::get_alloc(), &m_brdf_lut.m_image_view));

        vk::SamplerCreateInfo sampler_ci {};
        sampler_ci.magFilter = vk::Filter::eLinear;
        sampler_ci.minFilter = vk::Filter::eLinear;
        sampler_ci.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_ci.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        sampler_ci.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        sampler_ci.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        sampler_ci.mipLodBias = .0F;
        sampler_ci.maxAnisotropy = 1.F;
        sampler_ci.minLod = .0F;
        sampler_ci.maxLod = 1.F;
        sampler_ci.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        vkcheck(dvc.createSampler(&sampler_ci, vkb::get_alloc(), &sampler));

        vk::DescriptorImageInfo image_info {};
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_brdf_lut.m_image_view;
        image_info.sampler = sampler;

        vk::AttachmentDescription attachment {};
        attachment.format = brfd_lut_fmt;
        attachment.samples = vk::SampleCountFlagBits::e1;
        attachment.loadOp = vk::AttachmentLoadOp::eClear;
        attachment.storeOp = vk::AttachmentStoreOp::eStore;
        attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment.initialLayout = vk::ImageLayout::eUndefined;
        attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::AttachmentReference color_reference {};
        color_reference.attachment = 0;
        color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass {};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_reference;

        eastl::array<vk::SubpassDependency, 2> dependencies {};
        dependencies[0].srcSubpass = vk::SubpassExternal;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = vk::SubpassExternal;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo render_pass_ci {};
        render_pass_ci.attachmentCount = 1;
        render_pass_ci.pAttachments = &attachment;
        render_pass_ci.subpassCount = 1;
        render_pass_ci.pSubpasses = &subpass;
        render_pass_ci.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_ci.pDependencies = dependencies.data();

        vk::RenderPass render_pass {};
        vkcheck(dvc.createRenderPass(&render_pass_ci, vkb::get_alloc(), &render_pass));

        vk::FramebufferCreateInfo framebuffer_ci {};
        framebuffer_ci.renderPass = render_pass;
        framebuffer_ci.attachmentCount = 1;
        framebuffer_ci.pAttachments = &m_brdf_lut.m_image_view;
        framebuffer_ci.width = brdf_lut_size();
        framebuffer_ci.height = brdf_lut_size();
        framebuffer_ci.layers = 1;

        vk::Framebuffer framebuffer {};
        vkcheck(dvc.createFramebuffer(&framebuffer_ci, vkb::get_alloc(), &framebuffer));

        vk::DescriptorSetLayout descriptor_set_layout {};
        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_ci {};
        vkcheck(dvc.createDescriptorSetLayout(&descriptor_set_layout_ci, vkb::get_alloc(), &descriptor_set_layout));

        vk::DescriptorPool descriptor_pool {};
        vk::DescriptorPoolSize pool_size {};
        pool_size.type = vk::DescriptorType::eCombinedImageSampler;
        pool_size.descriptorCount = 1;
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.maxSets = 2;
        descriptor_pool_ci.poolSizeCount = 1;
        descriptor_pool_ci.pPoolSizes = &pool_size;
        vkcheck(dvc.createDescriptorPool(&descriptor_pool_ci, vkb::get_alloc(), &descriptor_pool));

        vk::DescriptorSet descriptor_set {};
        vk::DescriptorSetAllocateInfo descriptor_set_ai {};
        descriptor_set_ai.descriptorPool = descriptor_pool;
        descriptor_set_ai.descriptorSetCount = 1;
        descriptor_set_ai.pSetLayouts = &descriptor_set_layout;
        vkcheck(dvc.allocateDescriptorSets(&descriptor_set_ai, &descriptor_set));

        vk::PipelineLayout pipeline_layout {};
        vk::PipelineLayoutCreateInfo pipeline_layout_ci {};
        pipeline_layout_ci.setLayoutCount = 1;
        pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;
        vkcheck(dvc.createPipelineLayout(&pipeline_layout_ci, vkb::get_alloc(), &pipeline_layout));

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci {};
        input_assembly_ci.topology = vk::PrimitiveTopology::eTriangleList;
        input_assembly_ci.primitiveRestartEnable = vk::False;

        vk::PipelineRasterizationStateCreateInfo rasterization_ci {};
        rasterization_ci.depthClampEnable = vk::False;
        rasterization_ci.rasterizerDiscardEnable = vk::False;
        rasterization_ci.polygonMode = vk::PolygonMode::eFill;
        rasterization_ci.lineWidth = 1.F;
        rasterization_ci.cullMode = vk::CullModeFlagBits::eNone;
        rasterization_ci.frontFace = vk::FrontFace::eCounterClockwise;

        vk::PipelineColorBlendAttachmentState color_blend_attachment {};
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo color_blend_ci {};
        color_blend_ci.logicOpEnable = vk::False;
        color_blend_ci.attachmentCount = 1;
        color_blend_ci.pAttachments = &color_blend_attachment;

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_ci {};
        depth_stencil_ci.depthTestEnable = vk::False;
        depth_stencil_ci.depthWriteEnable = vk::False;
        depth_stencil_ci.depthCompareOp = vk::CompareOp::eLessOrEqual;

        vk::PipelineViewportStateCreateInfo viewport_ci {};
        viewport_ci.viewportCount = 1;
        viewport_ci.scissorCount = 1;

        vk::PipelineMultisampleStateCreateInfo multisample_ci {};
        multisample_ci.rasterizationSamples = vk::SampleCountFlagBits::e1;

        eastl::array<vk::DynamicState, 2> dynamic_states {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamic_state_ci {};
        dynamic_state_ci.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state_ci.pDynamicStates = dynamic_states.data();

        vk::PipelineVertexInputStateCreateInfo vertex_input_ci {};

        eastl::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/gen_brdf_lut.vert", shader_stage::vertex})->get_stage_info(),
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/gen_brdf_lut.frag", shader_stage::fragment})->get_stage_info()
        };

        vk::Pipeline shader_pipeline {};
        vk::GraphicsPipelineCreateInfo pipeline_ci {};
        pipeline_ci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_ci.pStages = shader_stages.data();
        pipeline_ci.pVertexInputState = &vertex_input_ci;
        pipeline_ci.pInputAssemblyState = &input_assembly_ci;
        pipeline_ci.pRasterizationState = &rasterization_ci;
        pipeline_ci.pColorBlendState = &color_blend_ci;
        pipeline_ci.pMultisampleState = &multisample_ci;
        pipeline_ci.pViewportState = &viewport_ci;
        pipeline_ci.pDepthStencilState = &depth_stencil_ci;
        pipeline_ci.pDynamicState = &dynamic_state_ci;
        pipeline_ci.layout = pipeline_layout;
        pipeline_ci.renderPass = render_pass;
        pipeline_ci.subpass = 0;
        vkcheck(dvc.createGraphicsPipelines(nullptr, 1, &pipeline_ci, vkb::get_alloc(), &shader_pipeline));

        // now render
        vk::ClearValue clear_value {};
        clear_value.color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };

        vk::RenderPassBeginInfo render_pass_bi {};
        render_pass_bi.renderPass = render_pass;
        render_pass_bi.framebuffer = framebuffer;
        render_pass_bi.renderArea.extent = { brdf_lut_size(), brdf_lut_size() };
        render_pass_bi.clearValueCount = 1;
        render_pass_bi.pClearValues = &clear_value;

        vk::CommandBuffer cmd = vkb::ctx().start_command_buffer<vk::QueueFlagBits::eGraphics>();
        cmd.beginRenderPass(&render_pass_bi, vk::SubpassContents::eInline);

        vk::Viewport viewport {};
        viewport.width = static_cast<float>(brdf_lut_size());
        viewport.height = static_cast<float>(brdf_lut_size());
        viewport.minDepth = .0F;
        viewport.maxDepth = 1.F;
        cmd.setViewport(0, 1, &viewport);

        vk::Rect2D scissor {};
        scissor.extent = { brdf_lut_size(), brdf_lut_size() };
        cmd.setScissor(0, 1, &scissor);

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, shader_pipeline);
        cmd.draw(3, 1, 0, 0);

        cmd.endRenderPass();
        vkb::ctx().flush_command_buffer<vk::QueueFlagBits::eGraphics>(cmd);

        vkcheck(dvc.waitIdle());

        dvc.destroyPipeline(shader_pipeline, vkb::get_alloc());
        dvc.destroyPipelineLayout(pipeline_layout, vkb::get_alloc());
        dvc.destroyRenderPass(render_pass, vkb::get_alloc());
        dvc.destroyFramebuffer(framebuffer, vkb::get_alloc());
        dvc.destroyDescriptorSetLayout(descriptor_set_layout, vkb::get_alloc());
        dvc.destroyDescriptorPool(descriptor_pool, vkb::get_alloc());

        log_info("BRDF LUT generated in {} ms", std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::high_resolution_clock::now() - now).count());
    }
}
