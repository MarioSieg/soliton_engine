// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"

#include <execution>
#include <mimalloc.h>

#include "imgui/text_editor.hpp"
#include "imgui/implot.h"
#include "material.hpp"

using platform::platform_subsystem;

namespace graphics {
    using vkb::context;

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");

        ImGui::SetAllocatorFunctions(
            +[](size_t size, [[maybe_unused]] void* usr) -> void* {
                return mi_malloc(size);
            },
            +[](void* ptr, [[maybe_unused]] void* usr) -> void {
                mi_free(ptr);
            }
        );
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.IniFilename = nullptr;

        GLFWwindow* window = platform_subsystem::get_glfw_window();
        context::s_instance = std::make_unique<context>(window); // Create Vulkan context

        material::init_static_resources();

        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
        create_descriptor_sets();
        create_pipeline();
    }

    graphics_subsystem::~graphics_subsystem() {
        vkcheck(context::s_instance->get_device().get_logical_device().waitIdle());

        for (auto& [buffer, descriptor_set] : m_uniforms) {
            buffer.~buffer();
        }
        vkb_vk_device().destroyDescriptorSetLayout(m_descriptor_set_layout, &vkb::s_allocator);
        vkb_vk_device().destroyPipelineLayout(m_pipeline_layout, &vkb::s_allocator);
        vkb_vk_device().destroyDescriptorPool(m_descriptor_pool, &vkb::s_allocator);
        vkb_vk_device().destroyPipeline(m_pipeline, &vkb::s_allocator);

        material::free_static_resources();

        context::s_instance.reset();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    [[nodiscard]] static auto get_main_camera() -> flecs::entity {
        if (const auto& scene = scene::get_active()) [[likely]] {
            const auto filter = scene->filter<const c_transform, c_camera>();
            if (filter.count() > 0) {
                return filter.first();
            }
        }
        return flecs::entity::null();
    }

    static constinit DirectX::XMFLOAT4X4A s_view_mtx;
    static constinit DirectX::XMFLOAT4X4A s_proj_mtx;
    static constinit DirectX::XMFLOAT4X4A s_view_proj_mtx;
    static DirectX::BoundingFrustum s_frustum;
    static c_transform s_camera_transform;

    static auto update_main_camera(const float width, const float height) -> void {
        const flecs::entity main_cam = get_main_camera();
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
        const DirectX::XMMATRIX view = cam.compute_view(s_camera_transform);
        const DirectX::XMMATRIX proj = cam.compute_projection();
        XMStoreFloat4x4A(&s_view_mtx, view);
        XMStoreFloat4x4A(&s_proj_mtx, proj);
        XMStoreFloat4x4A(&s_view_proj_mtx, XMMatrixMultiply(view, proj));
        DirectX::BoundingFrustum::CreateFromMatrix(s_frustum, proj);
        s_frustum.Transform(s_frustum, XMMatrixInverse(nullptr, view));
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(context::s_instance->get_width());
        io.DisplaySize.y = static_cast<float>(context::s_instance->get_height());

        m_cmd_buf = context::s_instance->begin_frame(DirectX::XMFLOAT4{0.53f, 0.81f, 0.92f, 1.0f});

        return true;
    }

    HOTPROC static auto draw_mesh(
        const mesh& mesh,
        const vk::CommandBuffer cmd,
        const std::vector<material*>& mats,
        const vk::PipelineLayout layout
    ) -> void {
        constexpr vk::DeviceSize offsets = 0;
        cmd.bindVertexBuffers(0, 1, &mesh.get_vertex_buffer().get_buffer(), &offsets);
        cmd.bindIndexBuffer(mesh.get_index_buffer().get_buffer(), 0, mesh.is_index_32bit() ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
        if (mesh.get_primitives().size() <= mats.size()) { // we have at least one material for each primitive
            for (const mesh::primitive& prim : mesh.get_primitives()) {
                if (prim.dst_material_index >= mats.size()) [[unlikely]] {
                    log_error("Mesh has invalid material index");
                    continue;
                }
                cmd.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    layout,
                    1,
                    1,
                    &mats[prim.dst_material_index]->get_descriptor_set(),
                    0,
                    nullptr
                );
                cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
            }
        } else {
            cmd.drawIndexed(mesh.get_index_count(), 1, 0, 0, 0);
        }
    }

    HOTPROC auto graphics_subsystem::render_scene(const vk::CommandBuffer cmd_buf) -> void {
        const auto& scene = scene::get_active();
        if (!scene) [[unlikely]] return;
        if (!m_render_query.scene || &*scene != m_render_query.scene) [[unlikely]] { // Scene changed
            if (m_render_query.query) {
                m_render_query.query.destruct();
            }
            m_render_query.scene = &*scene;
            m_render_query.query = scene->query<const c_transform, const c_mesh_renderer>();
        }

        const auto& query = m_render_query.query;
        const DirectX::XMMATRIX vp = XMLoadFloat4x4A(&s_view_proj_mtx);
        const auto render = [&](const c_transform& transform, const c_mesh_renderer& renderer) {
            // Checks
            if (!renderer.mesh || renderer.materials.empty() || renderer.flags & render_flags::skip_rendering) [[unlikely]] {
                return;
            }

            const mesh& mesh = *renderer.mesh;

            const DirectX::XMMATRIX model = transform.compute_matrix();

            // Frustum Culling
            DirectX::BoundingOrientedBox obb {};
            obb.CreateFromBoundingBox(obb, mesh.get_aabb());
            obb.Transform(obb, model);
            if ((renderer.flags & render_flags::skip_frustum_culling) == 0) [[likely]] {
                if (s_frustum.Contains(obb) == DirectX::ContainmentType::DISJOINT) { // Object is culled
                    return;
                }
            }

            // Uniforms
            gpu_vertex_push_constants push_constants {};
            push_constants.model_view_proj = XMMatrixMultiply(model, vp);
            push_constants.normal_matrix = XMMatrixTranspose(XMMatrixInverse(nullptr, model));
            cmd_buf.pushConstants(
                m_pipeline_layout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(gpu_vertex_push_constants),
                &push_constants
            );

            draw_mesh(mesh, cmd_buf, renderer.materials, m_pipeline_layout);
        };
        query.iter().each(render);
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        if (!m_cmd_buf) [[unlikely]] return;

        const auto w = static_cast<float>(context::s_instance->get_width());
        const auto h = static_cast<float>(context::s_instance->get_height());
        update_main_camera(w, h);

        m_cmd_buf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_pipeline_layout,
            0,
            1,
            &m_uniforms[context::s_instance->get_current_frame()].descriptor_set,
            0,
            nullptr
        );
        m_cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

        render_scene(m_cmd_buf);

        ImGui::Render();
        context::s_instance->render_imgui(ImGui::GetDrawData(), m_cmd_buf);
        context::s_instance->end_frame(m_cmd_buf);

        c_camera::active_camera = flecs::entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {
        vkb_context().on_resize();
    }

    auto graphics_subsystem::on_start(scene& scene) -> void {
        flecs::entity camera = scene.spawn("MainCamera");
        camera.add<c_camera>();
    }

    auto graphics_subsystem::create_uniform_buffers() -> void {
        // Vertex shader uniform buffer block
        vk::BufferCreateInfo buffer_info {};
        buffer_info.size = sizeof(gpu_uniform_buffer);
        buffer_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;

        for (std::uint32_t i = 0; i < context::k_max_concurrent_frames; ++i) {
            m_uniforms[i].buffer.create(
                sizeof(gpu_uniform_buffer),
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

        constexpr std::array<vk::DescriptorSetLayoutBinding, 1> bindings {
            vk::DescriptorSetLayoutBinding {
                .binding = 0u,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1u,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
                .pImmutableSamplers = nullptr
            }
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_layout {};
        descriptor_layout.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor_layout.pBindings = bindings.data();
        vkcheck(device.createDescriptorSetLayout(&descriptor_layout, &vkb::s_allocator, &m_descriptor_set_layout));

        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
        // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
        vk::PipelineLayoutCreateInfo p_pipeline_layout_create_info {};
        const std::array<vk::DescriptorSetLayout, 2> set_layouts {
            m_descriptor_set_layout,
            material::get_descriptor_set_layout()
        };
        p_pipeline_layout_create_info.setLayoutCount = static_cast<std::uint32_t>(set_layouts.size());
        p_pipeline_layout_create_info.pSetLayouts = set_layouts.data();

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);
        p_pipeline_layout_create_info.pushConstantRangeCount = 1;
        p_pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
        vkcheck(device.createPipelineLayout(&p_pipeline_layout_create_info, &vkb::s_allocator, &m_pipeline_layout));
    }

    // Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
    auto graphics_subsystem::create_descriptor_pool() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        const vk::Device device = vkb_device.get_logical_device();

        const std::array<vk::DescriptorPoolSize, 1> sizes {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = static_cast<std::uint32_t>(m_uniforms.size())
            },
        };

        // Create the global descriptor pool
        // All descriptors used in this example are allocated from this pool
        vk::DescriptorPoolCreateInfo descriptor_pool_ci {};
        descriptor_pool_ci.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        descriptor_pool_ci.pPoolSizes = sizes.data();
        descriptor_pool_ci.maxSets = static_cast<std::uint32_t>(m_uniforms.size()); // Allocate one set for each frame
        vkcheck(device.createDescriptorPool(&descriptor_pool_ci, &vkb::s_allocator, &m_descriptor_pool));
    }

    // Shaders access data using descriptor sets that "point" at our uniform buffers
    // The descriptor sets make use of the descriptor set layouts created above
    auto graphics_subsystem::create_descriptor_sets() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        const vk::Device device = vkb_device.get_logical_device();

        for (std::uint32_t i = 0; i < context::k_max_concurrent_frames; ++i) {
            vk::DescriptorSetAllocateInfo alloc_info {};
            alloc_info.descriptorPool = m_descriptor_pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts = &m_descriptor_set_layout;
            vkcheck(device.allocateDescriptorSets(&alloc_info, &m_uniforms[i].descriptor_set));

            vk::DescriptorBufferInfo buffer_info {};
            buffer_info.buffer = m_uniforms[i].buffer.get_buffer();
            buffer_info.offset = 0;
            buffer_info.range = sizeof(gpu_uniform_buffer);

            const std::array<vk::WriteDescriptorSet, 1> write_descriptor_sets {
                vk::WriteDescriptorSet {
                    .dstSet = m_uniforms[i].descriptor_set,
                    .dstBinding = 0u,
                    .dstArrayElement = 0u,
                    .descriptorCount = 1u,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pImageInfo = nullptr,
                    .pBufferInfo = &buffer_info
                }
            };

            device.updateDescriptorSets(write_descriptor_sets.size(), write_descriptor_sets.data(), 0, nullptr);
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
        pipeline_ci.layout = m_pipeline_layout;
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
        rasterization_state.frontFace = vk::FrontFace::eCounterClockwise;
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

        // Push constants
        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);

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
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_ci, &vkb::s_allocator, &m_pipeline));
    }
}
