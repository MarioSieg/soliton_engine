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

        m_vs.emplace("triangle.vert");
        m_fs.emplace("triangle.frag");

        create_vertex_buffer();
        create_index_buffer();
        create_uniform_buffers();
        create_descriptor_set_layout();
        create_descriptor_pool();
        create_descriptor_sets();
        create_pipeline();
    }

    graphics_subsystem::~graphics_subsystem() {
        m_fs.reset();
        m_vs.reset();
        context::s_instance.reset();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    [[nodiscard]] static auto get_main_camera() -> entity {
        const auto& scene = scene::get_active();
        if (scene) [[likely]] {
            auto query = scene->filter<const c_transform, c_camera>();
            if (query.count() > 0) {
                return query.first();
            }
        }
        return entity::null();
    }

    static XMMATRIX m_mtx_view;
    static XMMATRIX m_mtx_proj;
    static XMMATRIX m_mtx_view_proj;
    static c_transform m_camera_transform;

    static auto update_main_camera(float width, float height) -> void {
        entity main_cam = get_main_camera();
        if (!main_cam.is_valid() || !main_cam.is_alive()) [[unlikely]] {
            log_warn("No camera found in scene");
            return;
        }
        c_camera::active_camera = main_cam;
        m_camera_transform = *main_cam.get<c_transform>();
        c_camera& cam = *main_cam.get_mut<c_camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        m_mtx_view = cam.compute_view(m_camera_transform);
        m_mtx_proj = cam.compute_projection();
        m_mtx_view_proj = XMMatrixMultiply(m_mtx_view, m_mtx_proj);
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(context::s_instance->get_width());
        io.DisplaySize.y = static_cast<float>(context::s_instance->get_height());

        cmd_buf = context::s_instance->begin_frame(DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f});

        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        if (!cmd_buf) [[unlikely]] return;

        const auto w = static_cast<float>(context::s_instance->get_width());
        const auto h = static_cast<float>(context::s_instance->get_height());
        update_main_camera(w, h);

        cpu_uniform_buffer ubo {};
        XMMATRIX model = XMMatrixIdentity();
        ubo.mvp = XMMatrixMultiply(model, m_mtx_view_proj);
        std::memcpy(uniforms[context::s_instance->get_current_frame()].mapped, &ubo, sizeof(cpu_uniform_buffer));

        cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &uniforms[context::s_instance->get_current_frame()].descriptor_set, 0, nullptr);
        cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        constexpr vk::DeviceSize offsets = 0;
        cmd_buf.bindVertexBuffers(0, 1, &vertices.buf, &offsets);
        cmd_buf.bindIndexBuffer(indices.buf, 0, vk::IndexType::eUint32);
        cmd_buf.drawIndexed(indices.count, 1, 0, 0, 1);


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

    struct staging_buffer final {
        vk::DeviceMemory mem {};
        vk::Buffer buf {};
    };

    auto graphics_subsystem::create_vertex_buffer() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        // Setup vertices
        std::vector<vertex> vb {
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };
        auto vb_size = static_cast<std::uint32_t>(vb .size()) * sizeof(vertex);

        staging_buffer staging {};
        vk::MemoryAllocateInfo alloc_info {};
        vk::MemoryRequirements mem_reqs {};
        void* dst {};

        vk::BufferCreateInfo buffer_info {};
        buffer_info.size = vb_size;
        buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc; // Buffer is used as the copy source
        // Create a host-visible buffer to copy the vertex data to (staging buffer)
        vkcheck(device.createBuffer(&buffer_info, &vkb::s_allocator, &staging.buf));
        device.getBufferMemoryRequirements(staging.buf, &mem_reqs);
        // Request a host visible memory type that can be used to copy our data do
        // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
        alloc_info.allocationSize = vb_size;
        alloc_info.memoryTypeIndex = vkb_device.get_mem_type_idx_or_panic(
            mem_reqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        vkcheck(device.allocateMemory(&alloc_info, &vkb::s_allocator, &staging.mem));
        // Map and copy
        vkcheck(device.mapMemory(staging.mem, 0, vb_size, {}, &dst));
        std::memcpy(dst, vb.data(), vb_size);
        device.unmapMemory(staging.mem);
        // Bind memory
        device.bindBufferMemory(staging.buf, staging.mem, 0);

        // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
        buffer_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst; // Buffer will be used as a vertex buffer and is the copy destination
        vkcheck(device.createBuffer(&buffer_info, &vkb::s_allocator, &vertices.buf));
        device.getBufferMemoryRequirements(vertices.buf, &mem_reqs);
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = vkb_device.get_mem_type_idx_or_panic(
            mem_reqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        vkcheck(device.allocateMemory(&alloc_info, &vkb::s_allocator, &vertices.mem));
        device.bindBufferMemory(vertices.buf, vertices.mem, 0);

        // Buffer copies have to be submitted to a queue, so we need a command buffer for them
        // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
        vk::CommandBuffer copy_cmd {};
        vk::CommandBufferAllocateInfo cmd_alloc_info {};
        cmd_alloc_info.commandPool = context::s_instance->get_command_pool();
        cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        cmd_alloc_info.commandBufferCount = 1;
        vkcheck(device.allocateCommandBuffers(&cmd_alloc_info, &copy_cmd));

        vk::CommandBufferBeginInfo cmd_begin_info {};
        cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        vkcheck(copy_cmd.begin(&cmd_begin_info));
        vk::BufferCopy copy_region {};
        copy_region.size = vb_size;
        copy_cmd.copyBuffer(staging.buf, vertices.buf, 1, &copy_region);
        copy_cmd.end();

        vk::FenceCreateInfo fence_info {};
        vk::Fence fence {};
        vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));

        vk::SubmitInfo submit_info {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &copy_cmd;
        vkcheck(vkb_device.get_graphics_queue().submit(1, &submit_info, fence));
        vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));

        device.destroyFence(fence, &vkb::s_allocator);
        device.freeCommandBuffers(context::s_instance->get_command_pool(), 1, &copy_cmd);
        device.destroyBuffer(staging.buf, &vkb::s_allocator);
        device.freeMemory(staging.mem, &vkb::s_allocator);
    }

    auto graphics_subsystem::create_index_buffer() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        // Setup indices
        std::vector<uint32_t> ib { 0, 1, 2 };
        indices.count = static_cast<std::uint32_t>(ib.size());
        const auto ib_size = indices.count * sizeof(std::uint32_t);

        staging_buffer staging {};
        vk::MemoryAllocateInfo alloc_info {};
        vk::MemoryRequirements mem_reqs {};
        void* dst {};

        vk::BufferCreateInfo buffer_info {};
        buffer_info.size = ib_size;
        buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc; // Buffer is used as the copy source
        // Create a host-visible buffer to copy the index data to (staging buffer)
        vkcheck(device.createBuffer(&buffer_info, &vkb::s_allocator, &staging.buf));
        device.getBufferMemoryRequirements(staging.buf, &mem_reqs);
        // Request a host visible memory type that can be used to copy our data do
        // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
        alloc_info.allocationSize = ib_size;
        alloc_info.memoryTypeIndex = vkb_device.get_mem_type_idx_or_panic(
            mem_reqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        vkcheck(device.allocateMemory(&alloc_info, &vkb::s_allocator, &staging.mem));
        // Map and copy
        vkcheck(device.mapMemory(staging.mem, 0, ib_size, {}, &dst));
        std::memcpy(dst, ib.data(), ib_size);
        device.unmapMemory(staging.mem);
        // Bind memory
        device.bindBufferMemory(staging.buf, staging.mem, 0);

        // Create a device local buffer to which the (host local) index data will be copied and which will be used for rendering
        buffer_info.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst; // Buffer will be used as a vertex buffer and is the copy destination
        vkcheck(device.createBuffer(&buffer_info, &vkb::s_allocator, &indices.buf));
        device.getBufferMemoryRequirements(indices.buf, &mem_reqs);
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = vkb_device.get_mem_type_idx_or_panic(
            mem_reqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        vkcheck(device.allocateMemory(&alloc_info, &vkb::s_allocator, &indices.mem));
        device.bindBufferMemory(indices.buf, indices.mem, 0);

        // Buffer copies have to be submitted to a queue, so we need a command buffer for them
        // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
        vk::CommandBuffer copy_cmd {};
        vk::CommandBufferAllocateInfo cmd_alloc_info {};
        cmd_alloc_info.commandPool = context::s_instance->get_command_pool();
        cmd_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        cmd_alloc_info.commandBufferCount = 1;
        vkcheck(device.allocateCommandBuffers(&cmd_alloc_info, &copy_cmd));

        vk::CommandBufferBeginInfo cmd_begin_info {};
        cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        vkcheck(copy_cmd.begin(&cmd_begin_info));
        vk::BufferCopy copy_region {};
        copy_region.size = ib_size;
        copy_cmd.copyBuffer(staging.buf, indices.buf, 1, &copy_region);
        copy_cmd.end();

        vk::FenceCreateInfo fence_info {};
        vk::Fence fence {};
        vkcheck(device.createFence(&fence_info, &vkb::s_allocator, &fence));

        vk::SubmitInfo submit_info {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &copy_cmd;
        vkcheck(vkb_device.get_graphics_queue().submit(1, &submit_info, fence));
        vkcheck(device.waitForFences(1, &fence, vk::True, std::numeric_limits<std::uint64_t>::max()));

        device.destroyFence(fence, &vkb::s_allocator);
        device.freeCommandBuffers(context::s_instance->get_command_pool(), 1, &copy_cmd);
        device.destroyBuffer(staging.buf, &vkb::s_allocator);
        device.freeMemory(staging.mem, &vkb::s_allocator);
    }

    auto graphics_subsystem::create_uniform_buffers() -> void {
        const vkb::device& vkb_device = context::s_instance->get_device();
        vk::Device device = vkb_device.get_logical_device();

        vk::MemoryRequirements mem_reqs {};
        // Vertex shader uniform buffer block
        vk::BufferCreateInfo buffer_info {};
        buffer_info.size = sizeof(cpu_uniform_buffer);
        buffer_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;

        for (std::uint32_t i = 0; i < context::k_max_concurrent_frames; ++i) {
            vkcheck(device.createBuffer(&buffer_info, nullptr, &uniforms[i].buf));
            // Get memory requirements including size, alignment and memory type
            device.getBufferMemoryRequirements(uniforms[i].buf, &mem_reqs);
            vk::MemoryAllocateInfo alloc_info {};
            alloc_info.allocationSize = mem_reqs.size;
            // Get the memory type index that supports host visible memory access
            // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
            // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
            // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
            alloc_info.memoryTypeIndex = vkb_device.get_mem_type_idx_or_panic(
                mem_reqs.memoryTypeBits,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );
            vkcheck(device.allocateMemory(&alloc_info, &vkb::s_allocator, &uniforms[i].mem));
            device.bindBufferMemory(uniforms[i].buf, uniforms[i].mem, 0);
            // We map the buffer once, so we can update it without having to map it again
            device.mapMemory(uniforms[i].mem, 0, sizeof(cpu_uniform_buffer), {}, reinterpret_cast<void**>(&uniforms[i].mapped));
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
            buffer_info.buffer = uniforms[i].buf;
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
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
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

        // Vertex input binding
        // This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
        vk::VertexInputBindingDescription vertex_input_binding {};
        vertex_input_binding.binding = 0;
        vertex_input_binding.stride = sizeof(vertex);
        vertex_input_binding.inputRate = vk::VertexInputRate::eVertex;

        // Input attribute bindings describe shader attribute locations and memory layouts
        std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attributes {};
        // These match the following shader layout (see triangle.vert):
        //	layout (location = 0) in vec3 inPos;
        //	layout (location = 1) in vec3 inColor;
        // Attribute location 0: Position
        vertex_input_attributes[0].binding = 0;
        vertex_input_attributes[0].location = 0;
        vertex_input_attributes[0].format = vk::Format::eR32G32B32Sfloat;
        vertex_input_attributes[0].offset = offsetof(vertex, pos);
        // Attribute location 1: Color
        vertex_input_attributes[1].binding = 0;
        vertex_input_attributes[1].location = 1;
        vertex_input_attributes[1].format = vk::Format::eR32G32B32Sfloat;
        vertex_input_attributes[1].offset = offsetof(vertex, color);

        // Vertex input state used for pipeline creation
        vk::PipelineVertexInputStateCreateInfo vertex_input_state {};
        vertex_input_state.vertexBindingDescriptionCount = 1;
        vertex_input_state.pVertexBindingDescriptions = &vertex_input_binding;
        vertex_input_state.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_input_attributes.size());
        vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes.data();

        // Shaders
        std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {};
        shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
        shader_stages[0].module = m_vs->get_module();
        shader_stages[0].pName = "main";
        shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
        shader_stages[1].module = m_fs->get_module();
        shader_stages[1].pName = "main";

        // Set pipeline shader stage info
        pipeline_ci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
        pipeline_ci.pStages = shader_stages.data();

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

        // Shader modules are no longer needed once the graphics pipeline has been created
        device.destroyShaderModule(m_vs->get_module(), &vkb::s_allocator);
        device.destroyShaderModule(m_fs->get_module(), &vkb::s_allocator);
    }
}
