// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "command_buffer.hpp"
#include "context.hpp"

#include "../graphics_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"

namespace lu::vkb {
    static constinit std::atomic_uint32_t s_draw_call_count;
    static constinit std::atomic_uint32_t s_vertex_count;

    static auto validate_queue_type(const vk::QueueFlagBits flags) -> void {
        passert(flags == vk::QueueFlagBits::eGraphics || flags == vk::QueueFlagBits::eCompute || flags == vk::QueueFlagBits::eTransfer);
    }

    command_buffer::command_buffer(
        const vk::CommandPool pool,
        const vk::CommandBuffer cmd,
        const vk::Queue queue,
        const vk::QueueFlagBits queue_flags
    ) : m_pool{pool}, m_cmd{cmd}, m_queue{queue}, m_queue_flags{queue_flags}, m_is_owned{false} {
        validate_queue_type(queue_flags);
    }

    command_buffer::command_buffer(const vk::QueueFlagBits queue_flags, const vk::CommandBufferLevel level) : m_queue_flags{queue_flags}, m_is_owned{true} {
        validate_queue_type(queue_flags);
        vk::CommandBufferAllocateInfo cmd_alloc_info {};
        switch (queue_flags) {
            case vk::QueueFlagBits::eGraphics:
                m_pool = ctx().get_graphics_command_pool();
                m_queue = dvc().get_graphics_queue();
                break;
            case vk::QueueFlagBits::eCompute:
                m_pool = ctx().get_compute_command_pool();
                m_queue = dvc().get_compute_queue();
                break;
            case vk::QueueFlagBits::eTransfer:
                m_pool = ctx().get_transfer_command_pool();
                m_queue = dvc().get_transfer_queue();
                break;
            default:
                panic("Invalid queue type");
        }
        cmd_alloc_info.commandPool = m_pool;
        cmd_alloc_info.level = level;
        cmd_alloc_info.commandBufferCount = 1;
        vkcheck(vkdvc().allocateCommandBuffers(&cmd_alloc_info, &m_cmd)); // TODO: not thread safe
    }

    command_buffer::~command_buffer() {
        if (m_is_owned && m_pool) {
            vkdvc().freeCommandBuffers(m_pool, 1, &m_cmd);
        }
    }

    auto command_buffer::begin(
        const vk::CommandBufferUsageFlagBits usage,
        const vk::CommandBufferInheritanceInfo* const inheritance
    ) -> void {
        const vk::CommandBufferBeginInfo info {.flags = usage, .pInheritanceInfo = inheritance};
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

    auto command_buffer::begin_render_pass(const vk::RenderPassBeginInfo& info, const vk::SubpassContents contents) -> void {
        m_cmd.beginRenderPass(&info, contents);
    }

    auto command_buffer::begin_render_pass(const vk::RenderPass pass, const vk::SubpassContents contents) -> void {

    }

    auto command_buffer::end_render_pass() -> void {
        m_cmd.endRenderPass();
    }

    auto command_buffer::set_viewport(const float x, const float y, const float width, const float height, const float min_depth, const float max_depth) -> void {
        vk::Viewport viewport {};
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = min_depth;
        viewport.maxDepth = max_depth;
        m_cmd.setViewport(0, 1, &viewport);
    }

    auto command_buffer::set_scissor(const std::uint32_t width, const std::uint32_t height) -> void {
        vk::Rect2D scissor {};
        scissor.extent.width = width;
        scissor.extent.height = height;
        m_cmd.setScissor(0, 1, &scissor);
    }

    auto command_buffer::copy_buffer(const vk::Buffer src, const vk::Buffer dst, const vk::DeviceSize size, const vk::DeviceSize offset) -> void {
        vk::BufferCopy copy_region {};
        copy_region.size = size;
        copy_region.dstOffset = offset;
        m_cmd.copyBuffer(src, dst, 1, &copy_region);
    }

    command_buffer::command_buffer(command_buffer&& other) noexcept = default;
    auto command_buffer::operator=(command_buffer&& other) noexcept -> command_buffer& = default;

    auto command_buffer::execute_commands(const eastl::span<const vk::CommandBuffer> cmds) -> void {
        m_cmd.executeCommands(cmds.size(), cmds.data());
    }

    auto command_buffer::set_image_layout_barrier(
        const vk::Image image,
        const vk::ImageLayout old_layout,
        const vk::ImageLayout new_layout,
        const vk::ImageSubresourceRange range,
        const vk::PipelineStageFlags src_stage,
        const vk::PipelineStageFlags dst_stage
    ) -> void {
        vk::ImageMemoryBarrier barrier {};
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = image;
        barrier.subresourceRange = range;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (old_layout) {
            case vk::ImageLayout::eUndefined: barrier.srcAccessMask = {}; break;
            case vk::ImageLayout::ePreinitialized: barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eTransferDstOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eShaderReadOnlyOptimal: barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead; break;
            default:
                panic("unsupported layout transition!");
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (new_layout) {
            case vk::ImageLayout::eTransferDstOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; break;
            case vk::ImageLayout::eTransferSrcOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead; break;
            case vk::ImageLayout::eColorAttachmentOptimal: barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal: barrier.dstAccessMask = barrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite; break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                if (barrier.srcAccessMask == vk::AccessFlagBits {}) {
                    barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
                }
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                break;
            default:
                panic("unsupported layout transition!");
        }

        m_cmd.pipelineBarrier(
            src_stage,
            dst_stage,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
    }

    auto command_buffer::copy_buffer_to_image(const vk::Buffer buffer, const vk::Image image, const eastl::span<const vk::BufferImageCopy> regions) -> void {
        m_cmd.copyBufferToImage(
            buffer,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            regions.size(),
            regions.data()
        );
    }

    auto command_buffer::get_total_draw_calls() noexcept -> const std::atomic_uint32_t& {
        return s_draw_call_count;
    }

    auto command_buffer::get_total_draw_verts() noexcept -> const std::atomic_uint32_t& {
        return s_vertex_count;
    }
}
