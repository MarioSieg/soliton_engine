// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

// TODO (not that important): shorten code a lot by converting to derived class using new pipeline builder
// TODO (not that important): Port old GLM math code to DXM

#include "pbr_filter_processor.hpp"
#include "vulkancore/context.hpp"
#include "shader.hpp"
#include "../scripting/system_variable.hpp"

#include <numbers>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace lu::graphics {
    static const system_variable<std::uint32_t> brdf_lut_size {"renderer.brdf_lut_dim", {512u}};
    static const system_variable<std::uint32_t> irradiance_cube_size {"renderer.irradiance_cube_size", {64u}};
    static const system_variable<std::uint32_t> prefiltered_cube_size {"renderer.prefiltered_cube_size", {512u}};
    static const system_variable<std::uint32_t> prefiltered_cube_samples {"renderer.prefiltered_cube_samples", {32u}};

    static const eastl::array<glm::mat4, 6> k_cube_matrices {
        // POSITIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(-180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_X
        glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(-180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Y
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // POSITIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        // NEGATIVE_Z
        glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 0.0f, 1.0f))
    };


    pbr_filter_processor::pbr_filter_processor() : m_cube_mesh{"/engine_assets/meshes/skybox.gltf"} {
        m_environ_cube.emplace("/engine_assets/textures/hdr/gcanyon_cube.ktx");
        generate_irradiance_cube();
        generate_prefilter_cube();
        generate_brdf_lookup_table();
    }

    auto pbr_filter_processor::generate_irradiance_cube() -> void {
        passert(!m_irradiance_cube.has_value());

        stopwatch clock {};
        log_info("Generating irradiance cube...");

        const vk::Device dvc = vkb::vkdvc();
        const std::uint32_t dim = irradiance_cube_size();
        const std::uint32_t num_mips = texture::get_mip_count(dim, dim);

        m_irradiance_cube.emplace(
            texture_descriptor {
                .width = dim,
                .height = dim,
                .depth = 1,
                .miplevel_count = num_mips,
                .array_size = 6,
                .format = k_irradiance_cube_format,
                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                .flags = vk::ImageCreateFlagBits::eCubeCompatible,
                .is_cubemap = true,
                .sampler = {
                    .mag_filter = vk::Filter::eLinear,
                    .min_filter = vk::Filter::eLinear,
                    .mipmap_mode = vk::SamplerMipmapMode::eLinear,
                    .address_mode = vk::SamplerAddressMode::eClampToEdge,
                    .enable_anisotropy = false
                }
            },
            eastl::nullopt
        );

        vk::AttachmentDescription attachment_desc {};
        attachment_desc.format = k_irradiance_cube_format;
        attachment_desc.samples = vk::SampleCountFlagBits::e1;
        attachment_desc.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_desc.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_desc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_desc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment_desc.initialLayout = vk::ImageLayout::eUndefined;
        attachment_desc.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference color_ref {0, vk::ImageLayout::eColorAttachmentOptimal };

        vk::SubpassDescription subpass_desc = {};
        subpass_desc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_ref;

        eastl::array<vk::SubpassDependency, 2> dependencies {};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo render_pass_desc {};
        render_pass_desc.attachmentCount = 1;
        render_pass_desc.pAttachments = &attachment_desc;
        render_pass_desc.subpassCount = 1;
        render_pass_desc.pSubpasses = &subpass_desc;
        render_pass_desc.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_desc.pDependencies = dependencies.data();
        vk::RenderPass render_pass {};
        vkcheck(dvc.createRenderPass(&render_pass_desc, vkb::get_alloc(), &render_pass));

        struct {
            eastl::optional<texture> image {};
            vk::Framebuffer framebuffer {};
        } offscreen {};

        offscreen.image.emplace(
            texture_descriptor {
                .width = dim,
                .height = dim,
                .depth = 1,
                .miplevel_count = 1,
                .array_size = 1,
                .format = k_irradiance_cube_format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                .tiling = vk::ImageTiling::eOptimal,
                .sampler = {
                    .mag_filter = vk::Filter::eLinear,
                    .min_filter = vk::Filter::eLinear,
                    .mipmap_mode = vk::SamplerMipmapMode::eLinear,
                    .address_mode = vk::SamplerAddressMode::eClampToEdge,
                    .enable_anisotropy = false
                }
            },
            eastl::nullopt
        );

        vk::FramebufferCreateInfo framebuffer_desc {};
        framebuffer_desc.renderPass = render_pass;
        framebuffer_desc.attachmentCount = 1;
        framebuffer_desc.pAttachments = &offscreen.image->image_view();
        framebuffer_desc.width = dim;
        framebuffer_desc.height = dim;
        framebuffer_desc.layers = 1;
        vkcheck(dvc.createFramebuffer(&framebuffer_desc, vkb::get_alloc(), &offscreen.framebuffer));

        {
            vkb::command_buffer layout_cmd {vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary};
            layout_cmd.begin();
            layout_cmd.set_image_layout_barrier(
                offscreen.image->image(),
                vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal
            );
            layout_cmd.end();
            layout_cmd.flush();
        }

        vk::DescriptorSetLayout descriptor_set_layout {};
        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_ci {};
        descriptor_set_layout_ci.bindingCount = 1;
        vk::DescriptorSetLayoutBinding binding {};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        descriptor_set_layout_ci.pBindings = &binding;
        vkcheck(dvc.createDescriptorSetLayout(
            &descriptor_set_layout_ci,
            vkb::get_alloc(),
            &descriptor_set_layout)
        );

        vk::DescriptorPool descriptor_pool {};
        vk::DescriptorPoolSize pool_size {};
        pool_size.type = vk::DescriptorType::eCombinedImageSampler;
        pool_size.descriptorCount = 1;
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.maxSets = 1;
        descriptor_pool_ci.poolSizeCount = 1;
        descriptor_pool_ci.pPoolSizes = &pool_size;
        vkcheck(dvc.createDescriptorPool(
            &descriptor_pool_ci,
            vkb::get_alloc(),
            &descriptor_pool)
        );

        vk::DescriptorSet descriptor_set {};
        vk::DescriptorSetAllocateInfo descriptor_set_ai {};
        descriptor_set_ai.descriptorPool = descriptor_pool;
        descriptor_set_ai.descriptorSetCount = 1;
        descriptor_set_ai.pSetLayouts = &descriptor_set_layout;
        vkcheck(dvc.allocateDescriptorSets(&descriptor_set_ai, &descriptor_set));
        vk::WriteDescriptorSet write {};
        write.dstSet = descriptor_set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        vk::DescriptorImageInfo image_info {};
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_environ_cube->image_view();
        image_info.sampler = m_environ_cube->sampler();
        write.pImageInfo = &image_info;
        dvc.updateDescriptorSets(1, &write, 0, nullptr);

        struct {
            glm::mat4 mvp {};
            float delta_phi = (2.0f * std::numbers::pi_v<float>) / 180.0f;
            float delta_theta = (0.5f * std::numbers::pi_v<float>) / 64.0f;
        } push_block {};
        static_assert(sizeof(push_block) == sizeof(float)*(16+2));
        vk::PushConstantRange range { vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_block) };

        vk::PipelineLayout pipeline_layout {};
        vk::PipelineLayoutCreateInfo pipeline_layout_ci {};
        pipeline_layout_ci.setLayoutCount = 1;
        pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_ci.pushConstantRangeCount = 1;
        pipeline_layout_ci.pPushConstantRanges = &range;
        vkcheck(dvc.createPipelineLayout(&pipeline_layout_ci, vkb::get_alloc(), &pipeline_layout));

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci {};
        input_assembly_ci.topology = vk::PrimitiveTopology::eTriangleList;
        input_assembly_ci.primitiveRestartEnable = vk::False;

        vk::PipelineRasterizationStateCreateInfo rasterization_ci {};
        rasterization_ci.polygonMode = vk::PolygonMode::eFill;
        rasterization_ci.cullMode = vk::CullModeFlagBits::eNone;
        rasterization_ci.frontFace = vk::FrontFace::eCounterClockwise;
        rasterization_ci.lineWidth = 1.0f;

        vk::PipelineColorBlendAttachmentState color_blend_attachment {};
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo color_blend_ci {};
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

        constexpr eastl::array<vk::DynamicState, 2> dynamic_states {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamic_state_ci {};
        dynamic_state_ci.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state_ci.pDynamicStates = dynamic_states.data();

        vk::PipelineVertexInputStateCreateInfo vertex_input_ci {};
        vertex_input_ci.pVertexBindingDescriptions = k_vertex_binding_desc.data();
        vertex_input_ci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(k_vertex_binding_desc.size());
        vertex_input_ci.pVertexAttributeDescriptions = k_vertex_attrib_desc.data();
        vertex_input_ci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(k_vertex_attrib_desc.size());

        eastl::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/filter_env_cube.vert", shader_stage::vertex})->get_stage_info(),
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/irradiance_cube.frag", shader_stage::fragment})->get_stage_info()
        };

        vk::GraphicsPipelineCreateInfo pipeline_ci {};
        pipeline_ci.layout = pipeline_layout;
        pipeline_ci.renderPass = render_pass;
        pipeline_ci.pInputAssemblyState = &input_assembly_ci;
        pipeline_ci.pRasterizationState = &rasterization_ci;
        pipeline_ci.pColorBlendState = &color_blend_ci;
        pipeline_ci.pMultisampleState = &multisample_ci;
        pipeline_ci.pViewportState = &viewport_ci;
        pipeline_ci.pDepthStencilState = &depth_stencil_ci;
        pipeline_ci.pDynamicState = &dynamic_state_ci;
        pipeline_ci.pVertexInputState = &vertex_input_ci;
        pipeline_ci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_ci.pStages = shader_stages.data();

        vk::Pipeline shader_pipeline {};
        vkcheck(dvc.createGraphicsPipelines(nullptr, 1, &pipeline_ci, vkb::get_alloc(), &shader_pipeline));

        // Render:

        vk::ClearValue clear_value {};
        clear_value.color = vk::ClearColorValue { 0.0f, 0.0f, 0.2f, 0.0f };

        vk::RenderPassBeginInfo render_pass_bi {};
        render_pass_bi.renderPass = render_pass;
        render_pass_bi.framebuffer = offscreen.framebuffer;
        render_pass_bi.renderArea.extent = { dim, dim };
        render_pass_bi.clearValueCount = 1;
        render_pass_bi.pClearValues = &clear_value;

        vkb::command_buffer cmd {vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary};
        cmd.begin();

        cmd.set_viewport(0.0f, 0.0f, static_cast<float>(dim), static_cast<float>(dim));
        cmd.set_scissor(dim, dim);

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = num_mips;
        subresource_range.layerCount = 6;

        cmd.set_image_layout_barrier(
            m_irradiance_cube->image(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            subresource_range
        );

        for (std::uint32_t m = 0; m < num_mips; ++m) {
            for (std::uint32_t f = 0; f < 6; ++f) {
                const float w = static_cast<float>(dim) * std::pow(0.5f, m);
                const float h = static_cast<float>(dim) * std::pow(0.5f, m);
                log_info("Rendering face {} mip {} ({}x{})", f, m, w, h);
                cmd.set_viewport(
                    0.0f,
                    0.0f,
                    w,
                    h
                );
                cmd.begin_render_pass(render_pass_bi, vk::SubpassContents::eInline);
                push_block.mvp = glm::perspective(std::numbers::pi_v<float> / 2.0f, 1.0f, 0.1f, 512.0f) * k_cube_matrices[f];
                (*cmd).pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_block), &push_block);
                (*cmd).bindPipeline(vk::PipelineBindPoint::eGraphics, shader_pipeline);
                (*cmd).bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    pipeline_layout,
                    0,
                    1,
                    &descriptor_set,
                    0,
                    nullptr
                );
                cmd.draw_mesh(m_cube_mesh);
                cmd.end_render_pass();
                cmd.set_image_layout_barrier(
                    offscreen.image->image(),
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eTransferSrcOptimal
                );
                vk::ImageCopy copy {};
                copy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                copy.srcSubresource.baseArrayLayer = 0;
                copy.srcSubresource.mipLevel = 0;
                copy.srcSubresource.layerCount = 1;
                copy.srcOffset = vk::Offset3D {0, 0, 0};
                copy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                copy.dstSubresource.baseArrayLayer = f;
                copy.dstSubresource.mipLevel = m;
                copy.dstSubresource.layerCount = 1;
                copy.dstOffset = vk::Offset3D {0, 0, 0};
                copy.extent = vk::Extent3D {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), 1};
                (*cmd).copyImage(
                    offscreen.image->image(),
                    vk::ImageLayout::eTransferSrcOptimal,
                    m_irradiance_cube->image(),
                    vk::ImageLayout::eTransferDstOptimal,
                    1,
                    &copy
                );
                cmd.set_image_layout_barrier(
                    offscreen.image->image(),
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::ImageLayout::eColorAttachmentOptimal
                );
            }
        }

        cmd.set_image_layout_barrier(
            m_irradiance_cube->image(),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            subresource_range
        );
        cmd.end();
        cmd.flush();

        vkcheck(dvc.waitIdle());
        dvc.destroyPipeline(shader_pipeline, vkb::get_alloc());
        dvc.destroyPipelineLayout(pipeline_layout, vkb::get_alloc());
        dvc.destroyRenderPass(render_pass, vkb::get_alloc());
        dvc.destroyFramebuffer(offscreen.framebuffer, vkb::get_alloc());
        dvc.destroyDescriptorSetLayout(descriptor_set_layout, vkb::get_alloc());
        dvc.destroyDescriptorPool(descriptor_pool, vkb::get_alloc());

        log_info("Irradiance cube generated in {:.03f}s", clock.elapsed_secs());
    }

    auto pbr_filter_processor::generate_prefilter_cube() -> void {
        passert(!m_prefiltered_cube.has_value());

        stopwatch clock {};
        log_info("Generating prefiltered cube...");

        const vk::Device dvc = vkb::vkdvc();
        const std::uint32_t dim = prefiltered_cube_size();
        const std::uint32_t num_mips = texture::get_mip_count(dim, dim);

        m_prefiltered_cube.emplace(
            texture_descriptor {
                .width = dim,
                .height = dim,
                .depth = 1,
                .miplevel_count = num_mips,
                .array_size = 6,
                .format = k_irradiance_cube_format,
                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                .flags = vk::ImageCreateFlagBits::eCubeCompatible,
                .is_cubemap = true,
                .sampler = {
                    .mag_filter = vk::Filter::eLinear,
                    .min_filter = vk::Filter::eLinear,
                    .mipmap_mode = vk::SamplerMipmapMode::eLinear,
                    .address_mode = vk::SamplerAddressMode::eClampToEdge,
                    .enable_anisotropy = false
                }
            },
            eastl::nullopt
        );

        vk::AttachmentDescription attachment_desc {};
        attachment_desc.format = k_irradiance_cube_format;
        attachment_desc.samples = vk::SampleCountFlagBits::e1;
        attachment_desc.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_desc.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_desc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_desc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment_desc.initialLayout = vk::ImageLayout::eUndefined;
        attachment_desc.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference color_ref {0, vk::ImageLayout::eColorAttachmentOptimal };

        vk::SubpassDescription subpass_desc = {};
        subpass_desc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_ref;

        eastl::array<vk::SubpassDependency, 2> dependencies {};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo render_pass_desc {};
        render_pass_desc.attachmentCount = 1;
        render_pass_desc.pAttachments = &attachment_desc;
        render_pass_desc.subpassCount = 1;
        render_pass_desc.pSubpasses = &subpass_desc;
        render_pass_desc.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_desc.pDependencies = dependencies.data();
        vk::RenderPass render_pass {};
        vkcheck(dvc.createRenderPass(&render_pass_desc, vkb::get_alloc(), &render_pass));

        struct {
            eastl::optional<texture> image {};
            vk::Framebuffer framebuffer {};
        } offscreen {};

        offscreen.image.emplace(
            texture_descriptor {
                .width = dim,
                .height = dim,
                .depth = 1,
                .miplevel_count = 1,
                .array_size = 1,
                .format = k_irradiance_cube_format,
                .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                .tiling = vk::ImageTiling::eOptimal,
                .sampler = {
                    .mag_filter = vk::Filter::eLinear,
                    .min_filter = vk::Filter::eLinear,
                    .mipmap_mode = vk::SamplerMipmapMode::eLinear,
                    .address_mode = vk::SamplerAddressMode::eClampToEdge,
                    .enable_anisotropy = false
                }
            },
            eastl::nullopt
        );

        vk::FramebufferCreateInfo framebuffer_desc {};
        framebuffer_desc.renderPass = render_pass;
        framebuffer_desc.attachmentCount = 1;
        framebuffer_desc.pAttachments = &offscreen.image->image_view();
        framebuffer_desc.width = dim;
        framebuffer_desc.height = dim;
        framebuffer_desc.layers = 1;
        vkcheck(dvc.createFramebuffer(&framebuffer_desc, vkb::get_alloc(), &offscreen.framebuffer));

        {
            vkb::command_buffer layout_cmd {vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary};
            layout_cmd.begin();
            layout_cmd.set_image_layout_barrier(
                offscreen.image->image(),
                vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal
            );
            layout_cmd.end();
            layout_cmd.flush();
        }

        vk::DescriptorSetLayout descriptor_set_layout {};
        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_ci {};
        descriptor_set_layout_ci.bindingCount = 1;
        vk::DescriptorSetLayoutBinding binding {};
        binding.binding = 0;
        binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding.descriptorCount = 1;
        binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        descriptor_set_layout_ci.pBindings = &binding;
        vkcheck(dvc.createDescriptorSetLayout(
            &descriptor_set_layout_ci,
            vkb::get_alloc(),
            &descriptor_set_layout)
        );

        vk::DescriptorPool descriptor_pool {};
        vk::DescriptorPoolSize pool_size {};
        pool_size.type = vk::DescriptorType::eCombinedImageSampler;
        pool_size.descriptorCount = 1;
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.maxSets = 2;
        descriptor_pool_ci.poolSizeCount = 1;
        descriptor_pool_ci.pPoolSizes = &pool_size;
        vkcheck(dvc.createDescriptorPool(
            &descriptor_pool_ci,
            vkb::get_alloc(),
            &descriptor_pool)
        );

        vk::DescriptorSet descriptor_set {};
        vk::DescriptorSetAllocateInfo descriptor_set_ai {};
        descriptor_set_ai.descriptorPool = descriptor_pool;
        descriptor_set_ai.descriptorSetCount = 1;
        descriptor_set_ai.pSetLayouts = &descriptor_set_layout;
        vkcheck(dvc.allocateDescriptorSets(&descriptor_set_ai, &descriptor_set));
        vk::WriteDescriptorSet write {};
        write.dstSet = descriptor_set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        vk::DescriptorImageInfo image_info {};
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_environ_cube->image_view();
        image_info.sampler = m_environ_cube->sampler();
        write.pImageInfo = &image_info;
        dvc.updateDescriptorSets(1, &write, 0, nullptr);

        struct {
            glm::mat4 mvp {};
            float roughness = 0.0f;
            std::uint32_t num_samples = prefiltered_cube_samples();
        } push_block {};
        vk::PushConstantRange range { vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_block) };

        vk::PipelineLayout pipeline_layout {};
        vk::PipelineLayoutCreateInfo pipeline_layout_ci {};
        pipeline_layout_ci.setLayoutCount = 1;
        pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_ci.pushConstantRangeCount = 1;
        pipeline_layout_ci.pPushConstantRanges = &range;
        vkcheck(dvc.createPipelineLayout(&pipeline_layout_ci, vkb::get_alloc(), &pipeline_layout));

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci {};
        input_assembly_ci.topology = vk::PrimitiveTopology::eTriangleList;
        input_assembly_ci.primitiveRestartEnable = vk::False;

        vk::PipelineRasterizationStateCreateInfo rasterization_ci {};
        rasterization_ci.polygonMode = vk::PolygonMode::eFill;
        rasterization_ci.cullMode = vk::CullModeFlagBits::eNone;
        rasterization_ci.frontFace = vk::FrontFace::eCounterClockwise;
        rasterization_ci.lineWidth = 1.0f;

        vk::PipelineColorBlendAttachmentState color_blend_attachment {};
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo color_blend_ci {};
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

        constexpr eastl::array<vk::DynamicState, 2> dynamic_states {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamic_state_ci {};
        dynamic_state_ci.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state_ci.pDynamicStates = dynamic_states.data();

        vk::PipelineVertexInputStateCreateInfo vertex_input_ci {};
        vertex_input_ci.pVertexBindingDescriptions = k_vertex_binding_desc.data();
        vertex_input_ci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(k_vertex_binding_desc.size());
        vertex_input_ci.pVertexAttributeDescriptions = k_vertex_attrib_desc.data();
        vertex_input_ci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(k_vertex_attrib_desc.size());

        eastl::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/filter_env_cube.vert", shader_stage::vertex})->get_stage_info(),
            shader_cache::get().get_shader(shader_variant{"/engine_assets/shaders/src/prefilter_env_cube.frag", shader_stage::fragment})->get_stage_info()
        };

        vk::GraphicsPipelineCreateInfo pipeline_ci {};
        pipeline_ci.layout = pipeline_layout;
        pipeline_ci.renderPass = render_pass;
        pipeline_ci.pInputAssemblyState = &input_assembly_ci;
        pipeline_ci.pRasterizationState = &rasterization_ci;
        pipeline_ci.pColorBlendState = &color_blend_ci;
        pipeline_ci.pMultisampleState = &multisample_ci;
        pipeline_ci.pViewportState = &viewport_ci;
        pipeline_ci.pDepthStencilState = &depth_stencil_ci;
        pipeline_ci.pDynamicState = &dynamic_state_ci;
        pipeline_ci.pVertexInputState = &vertex_input_ci;
        pipeline_ci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_ci.pStages = shader_stages.data();

        vk::Pipeline shader_pipeline {};
        vkcheck(dvc.createGraphicsPipelines(nullptr, 1, &pipeline_ci, vkb::get_alloc(), &shader_pipeline));

        // Render:

        vk::ClearValue clear_value {};
        clear_value.color = vk::ClearColorValue { 0.0f, 0.0f, 0.2f, 0.0f };

        vk::RenderPassBeginInfo render_pass_bi {};
        render_pass_bi.renderPass = render_pass;
        render_pass_bi.framebuffer = offscreen.framebuffer;
        render_pass_bi.renderArea.extent = { dim, dim };
        render_pass_bi.clearValueCount = 1;
        render_pass_bi.pClearValues = &clear_value;

        vkb::command_buffer cmd {vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary};
        cmd.begin();

        cmd.set_viewport(0.0f, 0.0f, static_cast<float>(dim), static_cast<float>(dim));
        cmd.set_scissor(dim, dim);

        vk::ImageSubresourceRange subresource_range {};
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = num_mips;
        subresource_range.layerCount = 6;

        cmd.set_image_layout_barrier(
            m_prefiltered_cube->image(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            subresource_range
        );

        for (std::uint32_t m = 0; m < num_mips; ++m) {
            push_block.roughness = static_cast<float>(m) / static_cast<float>(num_mips - 1);
            for (std::uint32_t f = 0; f < 6; ++f) {
                const auto w = static_cast<std::uint32_t>(static_cast<float>(dim) * std::pow(0.5f, m));
                const auto h = static_cast<std::uint32_t>(static_cast<float>(dim) * std::pow(0.5f, m));
                log_info("Rendering face {} mip {} ({}x{}), R={}", f, m, w, h, push_block.roughness);
                cmd.set_viewport(
                    0.0f,
                    0.0f,
                    w,
                    h
                );
                cmd.begin_render_pass(render_pass_bi, vk::SubpassContents::eInline);
                push_block.mvp = glm::perspective(std::numbers::pi_v<float> / 2.0f, 1.0f, 0.1f, 512.0f) * k_cube_matrices[f];
                (*cmd).pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_block), &push_block);
                (*cmd).bindPipeline(vk::PipelineBindPoint::eGraphics, shader_pipeline);
                (*cmd).bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    pipeline_layout,
                    0,
                    1,
                    &descriptor_set,
                    0,
                    nullptr
                );
                cmd.draw_mesh(m_cube_mesh);
                cmd.end_render_pass();
                cmd.set_image_layout_barrier(
                    offscreen.image->image(),
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eTransferSrcOptimal
                );
                vk::ImageCopy copy {};
                copy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                copy.srcSubresource.baseArrayLayer = 0;
                copy.srcSubresource.mipLevel = 0;
                copy.srcSubresource.layerCount = 1;
                copy.srcOffset = vk::Offset3D {0, 0, 0};
                copy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                copy.dstSubresource.baseArrayLayer = f;
                copy.dstSubresource.mipLevel = m;
                copy.dstSubresource.layerCount = 1;
                copy.dstOffset = vk::Offset3D {0, 0, 0};
                copy.extent = vk::Extent3D {w, h, 1};
                (*cmd).copyImage(
                    offscreen.image->image(),
                    vk::ImageLayout::eTransferSrcOptimal,
                    m_prefiltered_cube->image(),
                    vk::ImageLayout::eTransferDstOptimal,
                    1,
                    &copy
                );
                cmd.set_image_layout_barrier(
                    offscreen.image->image(),
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageLayout::eTransferSrcOptimal,
                    vk::ImageLayout::eColorAttachmentOptimal
                );
            }
        }

        cmd.set_image_layout_barrier(
            m_prefiltered_cube->image(),
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            subresource_range
        );
        cmd.end();
        cmd.flush();

        vkcheck(dvc.waitIdle());
        dvc.destroyPipeline(shader_pipeline, vkb::get_alloc());
        dvc.destroyPipelineLayout(pipeline_layout, vkb::get_alloc());
        dvc.destroyRenderPass(render_pass, vkb::get_alloc());
        dvc.destroyFramebuffer(offscreen.framebuffer, vkb::get_alloc());
        dvc.destroyDescriptorSetLayout(descriptor_set_layout, vkb::get_alloc());
        dvc.destroyDescriptorPool(descriptor_pool, vkb::get_alloc());

        log_info("Irradiance prefiltered generated in {:.03f}s", clock.elapsed_secs());
    }

    auto pbr_filter_processor::generate_brdf_lookup_table() -> void {
        passert(!m_brdf_lut.has_value());

        stopwatch clock {};
        log_info("Generating BRDF LUT...");

        const vk::Device dvc = vkb::vkdvc();
        const std::uint32_t dim = brdf_lut_size();

        m_brdf_lut.emplace(
            texture_descriptor {
                .width = dim,
                .height = dim,
                .format = k_brfd_lut_format,
                .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
                .tiling = vk::ImageTiling::eOptimal,
                .sampler = {
                    .mag_filter = vk::Filter::eLinear,
                    .min_filter = vk::Filter::eLinear,
                    .mipmap_mode = vk::SamplerMipmapMode::eLinear,
                    .address_mode = vk::SamplerAddressMode::eClampToEdge,
                    .enable_anisotropy = false
                }
            },
            eastl::nullopt
        );

        vk::AttachmentDescription attachment {};
        attachment.format = k_brfd_lut_format;
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
        framebuffer_ci.pAttachments = &m_brdf_lut->image_view();
        framebuffer_ci.width = dim;
        framebuffer_ci.height = dim;
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
        render_pass_bi.renderArea.extent = { dim, dim };
        render_pass_bi.clearValueCount = 1;
        render_pass_bi.pClearValues = &clear_value;

        vkb::command_buffer cmd {vk::QueueFlagBits::eGraphics, vk::CommandBufferLevel::ePrimary};
        cmd.begin();
        cmd.begin_render_pass(render_pass_bi, vk::SubpassContents::eInline);
        cmd.set_viewport(0.0f, 0.0f, static_cast<float>(dim), static_cast<float>(dim));
        cmd.set_scissor(dim, dim);

        (*cmd).bindPipeline(vk::PipelineBindPoint::eGraphics, shader_pipeline);
        (*cmd).draw(3, 1, 0, 0);

        cmd.end_render_pass();
        cmd.end();
        cmd.flush();

        vkcheck(dvc.waitIdle());
        dvc.destroyPipeline(shader_pipeline, vkb::get_alloc());
        dvc.destroyPipelineLayout(pipeline_layout, vkb::get_alloc());
        dvc.destroyRenderPass(render_pass, vkb::get_alloc());
        dvc.destroyFramebuffer(framebuffer, vkb::get_alloc());
        dvc.destroyDescriptorSetLayout(descriptor_set_layout, vkb::get_alloc());
        dvc.destroyDescriptorPool(descriptor_pool, vkb::get_alloc());

        log_info("BRDF LUT generated in {:.03f}s", clock.elapsed_secs());
    }
}
