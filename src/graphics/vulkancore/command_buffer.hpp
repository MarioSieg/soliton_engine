// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

namespace soliton::graphics {
    class mesh;
    class material;
    class pipeline_base;
    class graphics_pipeline;
}

namespace soliton::vkb {
    template <typename T>
    concept is_push_constant_pod = requires {
        std::is_standard_layout_v<T>;
        std::is_trivial_v<T>;
        sizeof(T) <= 128; // TODO: Some GPUs have a limit of 256 bytes
        alignof(T) % 16 == 0;
    };

    class command_buffer final : public no_copy {
    public:
        explicit command_buffer(
            vk::CommandPool pool,
            vk::CommandBuffer cmd,
            vk::Queue queue,
            vk::QueueFlagBits queue_flags
        );
        explicit command_buffer(
            vk::QueueFlagBits queue_flags,
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
        );
        command_buffer(command_buffer&& other) noexcept;
        auto operator = (command_buffer&& other) noexcept -> command_buffer&;
        ~command_buffer();
        [[nodiscard]] constexpr auto operator *() const noexcept -> const vk::CommandBuffer& { return m_cmd; }
        constexpr operator vk::CommandBuffer() const noexcept { return m_cmd; }

        auto begin(
            vk::CommandBufferUsageFlagBits usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            const vk::CommandBufferInheritanceInfo* inheritance = nullptr
        ) -> void;
        auto end() -> void;
        auto flush() -> void;
        auto reset() -> void;

        auto begin_render_pass(const vk::RenderPassBeginInfo& info, vk::SubpassContents contents) -> void;
        auto begin_render_pass(vk::RenderPass pass, vk::SubpassContents contents) -> void;
        auto end_render_pass() -> void;
        auto set_viewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f) -> void;
        auto set_scissor(std::uint32_t width, std::uint32_t height) -> void;

        auto bind_vertex_buffer(vk::Buffer buffer, vk::DeviceSize offset = 0) -> void;
        auto bind_index_buffer(vk::Buffer buffer, bool index32, vk::DeviceSize offset = 0) -> void;
        auto bind_pipeline(const graphics::graphics_pipeline& pipeline) -> void;
        auto bind_material(const graphics::material& mat) -> void;
        auto bind_graphics_descriptor_set(vk::DescriptorSet set, std::uint32_t idx, const std::uint32_t* dynamic_off = nullptr) -> void;
        auto bind_mesh_buffers(const graphics::mesh& mesh) -> void;

        auto draw_mesh(const graphics::mesh& mesh) -> void;
        auto draw_mesh_with_materials(const graphics::mesh& mesh, eastl::span<graphics::material* const> mats) -> void;
        auto copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::DeviceSize offset = 0) -> void;

        auto push_consts_start() -> void;
        auto push_consts_raw(vk::ShaderStageFlagBits stage, const void* buf, std::size_t size) -> void;
        template <typename T> requires is_push_constant_pod<T>
        auto push_consts(const vk::ShaderStageFlagBits stage, const T& data) -> void {
            push_consts_raw(stage, &data, sizeof(T));
        }

        auto execute_commands(eastl::span<const vk::CommandBuffer> cmds) -> void;

        auto set_image_layout_barrier(
            vk::Image image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::ImageSubresourceRange range,
            vk::PipelineStageFlags src_stage = vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlags dst_stage = vk::PipelineStageFlagBits::eAllCommands
        ) -> void;

        auto set_image_layout_barrier(
            vk::Image image,
            vk::ImageAspectFlags aspect_mask,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::PipelineStageFlags src_stage = vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlags dst_stage = vk::PipelineStageFlagBits::eAllCommands
        ) -> void;

        auto copy_buffer_to_image(vk::Buffer buffer, vk::Image image, eastl::span<const vk::BufferImageCopy> regions) -> void;

        [[nodiscard]] auto command_pool() const noexcept -> vk::CommandPool { return m_pool; }
        [[nodiscard]] auto queue() const noexcept -> vk::Queue { return m_queue; }
        [[nodiscard]] auto queue_flags() const noexcept -> vk::QueueFlagBits { return m_queue_flags; }
        [[nodiscard]] auto bounded_pipeline() const noexcept -> const graphics::pipeline_base* { return m_bounded_pipeline; }
        [[nodiscard]] auto is_owned() const noexcept -> bool { return m_is_owned; }

        [[nodiscard]] static auto get_total_draw_calls() noexcept -> const std::atomic_uint32_t&;
        [[nodiscard]] static auto get_total_draw_verts() noexcept -> const std::atomic_uint32_t&;

    private:
        vk::CommandPool m_pool {};
        vk::CommandBuffer m_cmd {};
        vk::Queue m_queue {};
        vk::QueueFlagBits m_queue_flags {};
        const graphics::pipeline_base* m_bounded_pipeline {};
        std::size_t m_push_constant_offset {};
        bool m_is_owned {};
        bool m_push_consts_init {};
        bool m_was_used {};
    };
}
