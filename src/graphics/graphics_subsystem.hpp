// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "../vulkancore/context.hpp"
#include "../vulkancore/shader.hpp"
#include "../vulkancore/buffer.hpp"

namespace graphics {
    struct vertex final {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 uv;
        XMFLOAT3 tangent;
        XMFLOAT3 bitangent;
    };

    struct uniform_buffer final {
        vkb::buffer buffer {};
        vk::DescriptorSet descriptor_set {};
    };

    struct cpu_uniform_buffer final {
        XMMATRIX mvp;
    };
    static_assert(sizeof(cpu_uniform_buffer) % 4 == 0);

    class graphics_subsystem final : public subsystem {
    public:
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
        graphics_subsystem();
        ~graphics_subsystem() override;

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

    private:
        auto create_vertex_buffer() -> void;
        auto create_index_buffer() -> void;
        auto create_uniform_buffers() -> void;
        auto create_descriptor_set_layout() -> void;
        auto create_descriptor_pool() -> void;
        auto create_descriptor_sets() -> void;
        auto create_pipeline() -> void;

        vk::CommandBuffer cmd_buf = nullptr;
        std::optional<vkb::shader> m_vs {};
        std::optional<vkb::shader> m_fs {};
        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_index_buffer {};
        std::uint32_t m_index_count = 0;
        std::array<uniform_buffer, vkb::context::k_max_concurrent_frames> uniforms {};
        vk::DescriptorSetLayout descriptor_set_layout {};
        vk::PipelineLayout pipeline_layout {};
        vk::DescriptorPool descriptor_pool {};
        vk::Pipeline pipeline {};
    };
}
