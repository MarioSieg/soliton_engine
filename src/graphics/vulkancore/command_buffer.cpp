// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "command_buffer.hpp"
#include "context.hpp"

#include "../graphics_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"

namespace lu::vkb {
    static constinit std::atomic_uint32_t s_draw_call_count;
    static constinit std::atomic_uint32_t s_vertex_count;

    command_buffer::command_buffer(const vk::CommandBuffer cmd, const vk::Queue queue, const vk::QueueFlagBits queue_flags) noexcept {
        m_cmd = cmd;
        m_queue = queue;
        m_queue_flags = queue_flags;
        bool valid_queue = queue_flags == vk::QueueFlagBits::eGraphics;
        valid_queue |= queue_flags == vk::QueueFlagBits::eCompute;
        valid_queue |= queue_flags == vk::QueueFlagBits::eTransfer;
        passert(valid_queue);
    }

    auto command_buffer::begin(const vk::CommandBufferUsageFlagBits usage) -> void {
        const vk::CommandBufferBeginInfo info {.flags = usage};
        vkcheck(m_cmd.begin(&info));
    }

    auto command_buffer::end() -> void {
        vkcheck(m_cmd.end());
    }

    auto command_buffer::flush() -> void {
        const vk::Device device = vkdvc();
        constexpr vk::FenceCreateInfo fence_info {};
        vk::Fence fence {};
        vkcheck(device.createFence(&fence_info, vkb::get_alloc(), &fence));
        vk::SubmitInfo submit_info {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_cmd;
        const vk::Queue queue = m_queue;
        vkcheck(queue.submit(1, &submit_info, fence));// TODO: not thread safe, use transfer queue
        vkcheck(device.waitForFences(1, &fence, vk::True, eastl::numeric_limits<std::uint64_t>::max()));
        device.destroyFence(fence, vkb::get_alloc());
    }

    auto command_buffer::bind_vertex_buffer(const vk::Buffer buffer, const vk::DeviceSize offset) -> void {
        m_cmd.bindVertexBuffers(0, 1, &buffer, &offset);
    }

    auto command_buffer::bind_index_buffer(const vk::Buffer buffer, const bool index32, const vk::DeviceSize offset) -> void {
        m_cmd.bindIndexBuffer(buffer, offset, index32 ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
    }

    auto command_buffer::bind_mesh_buffers(const graphics::mesh& mesh) -> void {
        constexpr vk::DeviceSize offsets = 0;
        m_cmd.bindIndexBuffer(mesh.get_index_buffer().get_buffer(), 0, mesh.is_index_32bit() ? vk::IndexType::eUint32 : vk::IndexType::eUint16);
        m_cmd.bindVertexBuffers(0, 1, &mesh.get_vertex_buffer().get_buffer(), &offsets);
    }

    auto command_buffer::draw_mesh(const graphics::mesh& mesh) -> void {
        bind_mesh_buffers(mesh);
        std::uint32_t vertex_sum = 0;
        for (auto&& prim : mesh.get_primitives()) {
            m_cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
            vertex_sum += prim.vertex_count;
        }
        s_draw_call_count.fetch_add(mesh.get_primitives().size(), std::memory_order_relaxed);
        s_vertex_count.fetch_add(vertex_sum, std::memory_order_relaxed);
    }

    auto command_buffer::draw_mesh_with_materials(const graphics::mesh& mesh, const eastl::span<graphics::material* const> mats) -> void {
        bind_mesh_buffers(mesh);
        if (mesh.get_primitives().size() > mats.size()) [[unlikely]] {
            log_error("Not enough materials for mesh");
            return;
        }
        std::uint32_t vertex_sum = 0;
        for (std::size_t i = 0; i < mesh.get_primitives().size(); ++i) {
            bind_material(*mats[i]);
            const graphics::primitive& prim = mesh.get_primitives()[i];
            m_cmd.drawIndexed(prim.index_count, 1, prim.index_start, 0, 1);
        }
        s_draw_call_count.fetch_add(mesh.get_primitives().size(), std::memory_order_relaxed);
        s_vertex_count.fetch_add(vertex_sum, std::memory_order_relaxed);
    }

    auto command_buffer::bind_material(const graphics::material& mat) -> void {
        passert(m_bounded_pipeline != nullptr);
        m_cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_bounded_pipeline->get_layout(),
            0,
            1,
            &mat.get_descriptor_set(),
            0,
            nullptr
        );
    }

    auto command_buffer::bind_pipeline(const graphics::graphics_pipeline& pipeline) -> void {
        m_cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get_pipeline());
        m_bounded_pipeline = static_cast<const graphics::pipeline_base*>(&pipeline);
    }

    auto command_buffer::push_consts_start() -> void {
        m_push_constant_offset = 0;
    }

    auto command_buffer::push_consts_raw(const vk::ShaderStageFlagBits stage, const void* buf, std::size_t size) -> void {
        passert(m_bounded_pipeline != nullptr);
        m_cmd.pushConstants(
            m_bounded_pipeline->get_layout(),
            stage,
            m_push_constant_offset,
            size,
            buf
        );
        m_push_constant_offset += size;
    }
}
