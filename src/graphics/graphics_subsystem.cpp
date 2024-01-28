// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"

#include <execution>
#include <mimalloc.h>

#include "imgui/text_editor.hpp"
#include "imgui/implot.h"

using platform::platform_subsystem;

namespace graphics {
    using vkb::context;

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.IniFilename = nullptr;

        GLFWwindow* window = platform_subsystem::get_glfw_window();
        context::s_instance = std::make_unique<context>(window); // Create vulkan context

        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
        create_descriptor_sets();
        create_pipeline();
    }

    graphics_subsystem::~graphics_subsystem() {
        context::s_instance->get_device().get_logical_device().waitIdle();

        for (auto& [buffer, descriptor_set] : uniforms) {
            buffer.destroy();
        }
        vkb_vk_device().destroyDescriptorSetLayout(descriptor_set_layout, &vkb::s_allocator);
        vkb_vk_device().destroyPipelineLayout(pipeline_layout, &vkb::s_allocator);
        vkb_vk_device().destroyDescriptorPool(descriptor_pool, &vkb::s_allocator);
        vkb_vk_device().destroyPipeline(pipeline, &vkb::s_allocator);
        context::s_instance.reset();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    [[nodiscard]] static auto get_main_camera() -> entity {
        if (const auto& scene = scene::get_active()) [[likely]] {
            const auto filter = scene->filter<const c_transform, c_camera>();
            if (filter.count() > 0) {
                return filter.first();
            }
        }
        return entity::null();
    }

    static constinit XMFLOAT4X4A s_view_mtx;
    static constinit XMFLOAT4X4A s_proj_mtx;
    static constinit XMFLOAT4X4A s_view_proj_mtx;
    static BoundingFrustum s_frustum;
    static c_transform s_camera_transform;

    static auto update_main_camera(const float width, const float height) -> void {
        const entity main_cam = get_main_camera();
        if (!main_cam.is_valid() || !main_cam.is_alive()) [[unlikely]] {
            log_warn("No camera found in scene");
            return;
        }
        c_camera::active_camera = main_cam;
        s_camera_transform = *main_cam.get<c_transform>();
        c_camera& cam = *main_cam.get_mut<c_camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        XMMATRIX view = cam.compute_view(s_camera_transform);
        XMMATRIX proj = cam.compute_projection();
        XMStoreFloat4x4A(&s_view_mtx, view);
        XMStoreFloat4x4A(&s_proj_mtx, proj);
        XMStoreFloat4x4A(&s_view_proj_mtx, XMMatrixMultiply(view, proj));
        BoundingFrustum::CreateFromMatrix(s_frustum, proj);
        s_frustum.Transform(s_frustum, XMMatrixInverse(nullptr, view));
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(context::s_instance->get_width());
        io.DisplaySize.y = static_cast<float>(context::s_instance->get_height());

        cmd_buf = context::s_instance->begin_frame(DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f});

        return true;
    }

    HOTPROC auto graphics_subsystem::render_scene(const vk::CommandBuffer cmd_buf) const -> void {
        if (const auto& scene = scene::get_active()) [[likely]] {
            const auto query = scene->filter<const c_transform, const c_mesh_renderer>();
            const XMMATRIX vp = XMLoadFloat4x4A(&s_view_proj_mtx);
            const auto render = [&](const c_transform& transform, const c_mesh_renderer& renderer) {
                // Checks
                if (!renderer.mesh || renderer.flags & render_flags::skip_rendering) [[unlikely]] {
                    return;
                }

                const XMMATRIX model = transform.compute_matrix();

                // Frustum Culling
                BoundingOrientedBox obb {};
                obb.CreateFromBoundingBox(obb, renderer.mesh->get_aabb());
                obb.Transform(obb, model);
                if ((renderer.flags & render_flags::skip_frustum_culling) == 0) [[likely]] {
                    if (s_frustum.Contains(obb) == ContainmentType::INTERSECTS) { // Object is culled
                        return;
                    }
                }

                // Uniforms
                cpu_uniform_buffer ubo {};
                ubo.model_view_proj = XMMatrixMultiply(model, vp);
                ubo.normal_matrix = XMMatrixTranspose(XMMatrixInverse(nullptr, model));
                std::memcpy(uniforms[context::s_instance->get_current_frame()].buffer.get_mapped_ptr(), &ubo, sizeof(cpu_uniform_buffer));

                renderer.mesh->draw(cmd_buf);
            };
            query.iter().each(render);
        }
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        if (!cmd_buf) [[unlikely]] return;

        const auto w = static_cast<float>(context::s_instance->get_width());
        const auto h = static_cast<float>(context::s_instance->get_height());
        update_main_camera(w, h);

        cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &uniforms[context::s_instance->get_current_frame()].descriptor_set, 0, nullptr);
        cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

        render_scene(cmd_buf);

        ImGui::Render();
        context::s_instance->render_imgui(ImGui::GetDrawData(), cmd_buf);
        context::s_instance->end_frame(cmd_buf);

        c_camera::active_camera = entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {

    }

    void graphics_subsystem::on_start(scene& scene) {
        entity camera = scene.spawn("MainCamera");
        camera.add<c_camera>();
    }

    auto graphics_subsystem::create_uniform_buffers() -> void {
        // Vertex shader uniform buffer block
        vk::BufferCreateInfo buffer_info {};
        buffer_info.size = sizeof(cpu_uniform_buffer);
        buffer_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;

        for (std::uint32_t i = 0; i < context::k_max_concurrent_frames; ++i) {
            uniforms[i].buffer.create(
                sizeof(cpu_uniform_buffer),
                0,
                vk::BufferUsageFlagBits::eUniformBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                VMA_ALLOCATION_CREATE_MAPPED_BIT
            );
        }
    }

    auto graphics_subsystem::create_descriptor_set_layout() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        // Binding 0: Uniform buffer (Vertex shader)
        vk::DescriptorSetLayoutBinding layout_binding {};
        layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        layout_binding.descriptorCount = 1;
        layout_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
        layout_binding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo descriptor_layout {};
        descriptor_layout.bindingCount = 1;
        descriptor_layout.pBindings = &layout_binding;
        vkcheck(device.createDescriptorSetLayout(&descriptor_layout, &vkb::s_allocator, &descriptor_set_layout));

        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
        // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
        vk::PipelineLayoutCreateInfo p_pipeline_layout_create_info {};
        p_pipeline_layout_create_info.setLayoutCount = 1;
        p_pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
        vkcheck(device.createPipelineLayout(&p_pipeline_layout_create_info, &vkb::s_allocator, &pipeline_layout));
    }

    // Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
    auto graphics_subsystem::create_descriptor_pool() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        vk::DescriptorPoolSize pool_size {};
        pool_size.type = vk::DescriptorType::eUniformBuffer; // This example only one descriptor type (uniform buffer)
        pool_size.descriptorCount = static_cast<std::uint32_t>(uniforms.size()); // We have one buffer (and as such descriptor) per frame
        // For additional types you need to add new entries in the type count list
        // E.g. for two combined image samplers :
        // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // typeCounts[1].descriptorCount = 2;

        // Create the global descriptor pool
        // All descriptors used in this example are allocated from this pool
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.poolSizeCount = 1;
        descriptor_pool_ci.pPoolSizes = &pool_size;
        descriptor_pool_ci.maxSets = static_cast<std::uint32_t>(uniforms.size()); // Allocate one set for each frame
        vkcheck(device.createDescriptorPool(&descriptor_pool_ci, &vkb::s_allocator, &descriptor_pool));
    }

    // Shaders access data using descriptor sets that "point" at our uniform buffers
    // The descriptor sets make use of the descriptor set layouts created above
    auto graphics_subsystem::create_descriptor_sets() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        for (std::uint32_t i = 0; i < context::k_max_concurrent_frames; ++i) {
            vk::DescriptorSetAllocateInfo alloc_info {};
            alloc_info.descriptorPool = descriptor_pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &descriptor_set_layout;
            vkcheck(device.allocateDescriptorSets(&alloc_info, &uniforms[i].descriptor_set));

            // Update the descriptor set determining the shader binding points
            // For every binding point used in a shader there needs to be one
            // descriptor set matching that binding point
            vk::DescriptorBufferInfo buffer_info {};
            buffer_info.buffer = uniforms[i].buffer.get_buffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(cpu_uniform_buffer);

            vk::WriteDescriptorSet write_descriptor_set {};
            write_descriptor_set.dstSet = uniforms[i].descriptor_set;
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
            write_descriptor_set.pBufferInfo = &buffer_info;
            device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
        }
    }

    auto graphics_subsystem::create_pipeline() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        // Create the graphics pipeline used in this example
        // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
        // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
        // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

        vk::GraphicsPipelineCreateInfo pipeline_ci {};
        // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
        pipeline_ci.layout = pipeline_layout;
        // Renderpass this pipeline is attached to
        pipeline_ci.renderPass = context::s_instance->get_render_pass();

        // Construct the different states making up the pipeline

        // Input assembly state describes how primitives are assembled
        // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        input_assembly_state.topology = vk::PrimitiveTopology::eTriangleList;

        // Rasterization state
        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        rasterization_state.polygonMode = vk::PolygonMode::eFill;
        rasterization_state.cullMode = vk::CullModeFlagBits::eBack;
        rasterization_state.frontFace = vk::FrontFace::eClockwise;
        rasterization_state.depthClampEnable = vk::False;
        rasterization_state.rasterizerDiscardEnable = vk::False;
        rasterization_state.depthBiasEnable = vk::False;
        rasterization_state.lineWidth = 1.0f;

        // Color blend state describes how blend factors are calculated (if used)
        // We need one blend attachment state per color attachment (even if blending is not used)
        vk::PipelineColorBlendAttachmentState blend_attachment_state {};
        blend_attachment_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        blend_attachment_state.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;

        // Viewport state sets the number of viewports and scissor used in this pipeline
        // Note: This is actually overridden by the dynamic states (see below)
        vk::PipelineViewportStateCreateInfo viewport_state {};
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        // Enable dynamic states
        // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
        // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
        // For this example we will set the viewport and scissor using dynamic states
        constexpr std::array<vk::DynamicState, 2> dynamic_states {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        // Depth and stencil state containing depth and stencil compare and test operations
        // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {};
        depth_stencil_state.depthTestEnable = vk::True;
        depth_stencil_state.depthWriteEnable = vk::True;
        depth_stencil_state.depthCompareOp = vk::CompareOp::eLessOrEqual;
        depth_stencil_state.depthBoundsTestEnable = vk::False;
        depth_stencil_state.back.failOp = vk::StencilOp::eKeep;
        depth_stencil_state.back.passOp = vk::StencilOp::eKeep;
        depth_stencil_state.back.compareOp = vk::CompareOp::eAlways;
        depth_stencil_state.stencilTestEnable = vk::False;
        depth_stencil_state.front = depth_stencil_state.back;

        // Multi sampling state
        // This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        multisample_state.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisample_state.pSampleMask = nullptr;

        // Specifies the vertex input parameters for a pipeline


        vkb::shader vs{"triangle.vert"};
        vkb::shader fs{"triangle.frag"};

        // Shaders
        std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {};
        shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
        shader_stages[0].module = vs.get_module();
        shader_stages[0].pName = "main";
        shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
        shader_stages[1].module = fs.get_module();
        shader_stages[1].pName = "main";

        // Set pipeline shader stage info
        pipeline_ci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_ci.pStages = shader_stages.data();

        // Input attribute bindings describe shader attribute locations and memory layouts
        std::array<vk::VertexInputAttributeDescription, 5> vertex_input_attributes {};
        auto push_attribute = [&vertex_input_attributes, loc = 0](const vk::Format format, const std::uint32_t offset) mutable  {
            vertex_input_attributes[loc].location = loc;
            vertex_input_attributes[loc].binding = 0;
            vertex_input_attributes[loc].format = format;
            vertex_input_attributes[loc].offset = offset;
            ++loc;
        };
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, position));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, normal));
        push_attribute(vk::Format::eR32G32Sfloat, offsetof(mesh::vertex, uv));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, tangent));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, bitangent));

        // Vertex input binding
        // This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
        vk::VertexInputBindingDescription k_vertex_input_binding {};
        k_vertex_input_binding.binding = 0;
        k_vertex_input_binding.stride = static_cast<std::uint32_t>(sizeof(mesh::vertex));
        k_vertex_input_binding.inputRate = vk::VertexInputRate::eVertex;

        // Vertex input state used for pipeline creation
        vk::PipelineVertexInputStateCreateInfo vertex_input_state {};
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &k_vertex_input_binding;
        vertex_input_state.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_input_attributes.size());
        vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes.data();

        // Assign the pipeline states to the pipeline creation info structure
        pipeline_ci.pVertexInputState = &vertex_input_state;
        pipeline_ci.pInputAssemblyState = &input_assembly_state;
        pipeline_ci.pRasterizationState = &rasterization_state;
        pipeline_ci.pColorBlendState = &color_blend_state;
        pipeline_ci.pMultisampleState = &multisample_state;
        pipeline_ci.pViewportState = &viewport_state;
        pipeline_ci.pDepthStencilState = &depth_stencil_state;
        pipeline_ci.pDynamicState = &dynamic_state;

        // Create rendering pipeline using the specified states
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_ci, &vkb::s_allocator, &pipeline));
    }
}
