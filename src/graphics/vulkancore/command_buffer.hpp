// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "prelude.hpp"

namespace lu::graphics {
    class mesh;
    class material;
    class pipeline_base;
    class graphics_pipeline;
}

namespace lu::vkb {
    template <typename T>
    concept is_push_constant_pod = requires {
        std::is_standard_layout_v<T>;
        std::is_trivial_v<T>;
        sizeof(T) <= 128; // TODO: Some GPUs have a limit of 256 bytes
        alignof(T) <= 16;
    };

    class command_buffer final : public no_move, public no_copy {
    public:
        explicit command_buffer(vk::CommandPool pool, vk::CommandBuffer cmd, vk::Queue queue, vk::QueueFlagBits queue_flags);
        explicit command_buffer(vk::QueueFlagBits queue_flags, vk::CommandBufferLevel level);
        ~command_buffer();
        [[nodiscard]] constexpr auto get() const noexcept -> vk::CommandBuffer { return m_cmd; }
        constexpr operator vk::CommandBuffer() const noexcept { return m_cmd; }

        auto begin(vk::CommandBufferUsageFlagBits usage) -> void;
        auto end() -> void;
        auto flush() -> void;

        auto bind_vertex_buffer(vk::Buffer buffer, vk::DeviceSize offset = 0) -> void;
        auto bind_index_buffer(vk::Buffer buffer, bool index32, vk::DeviceSize offset = 0) -> void;
        auto bind_pipeline(const graphics::graphics_pipeline& pipeline) -> void;
        auto bind_material(const graphics::material& mat) -> void;
        auto bind_mesh_buffers(const graphics::mesh& mesh) -> void;

        auto draw_mesh(const graphics::mesh& mesh) -> void;
        auto draw_mesh_with_materials(const graphics::mesh& mesh, eastl::span<graphics::material* const> mats) -> void;

        auto push_consts_start() -> void;
        auto push_consts_raw(vk::ShaderStageFlagBits stage, const void* buf, std::size_t size) -> void;

        template <typename T> requires is_push_constant_pod<T>
        auto push_consts(const vk::ShaderStageFlagBits stage, const T& data) -> void {
            push_consts_raw(stage, &data, sizeof(T));
        }

    private:
        vk::CommandPool m_pool {};
        vk::CommandBuffer m_cmd {};
        vk::Queue m_queue {};
        vk::QueueFlagBits m_queue_flags {};
        const graphics::pipeline_base* m_bounded_pipeline {};
        std::size_t m_push_constant_offset {};
    };
}
