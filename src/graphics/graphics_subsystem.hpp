// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "../vulkancore/context.hpp"
#include "../vulkancore/shader.hpp"

namespace graphics {
    struct vertex final {
        XMFLOAT3 pos;
        XMFLOAT3 color;
    };

    struct uniform_buffer final {
        vk::DeviceMemory mem {};
        vk::Buffer buf {};
        vk::DescriptorSet descriptor_set {};
        std::uint8_t* mapped {};
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
        struct {
            vk::DeviceMemory mem {};
            vk::Buffer buf {};
        } vertices;
        struct {
            vk::DeviceMemory mem {};
            vk::Buffer buf {};
            std::uint32_t count {};
        } indices;
        std::array<uniform_buffer, vkb::context::k_max_concurrent_frames> uniforms {};
        vk::DescriptorSetLayout descriptor_set_layout {};
        vk::PipelineLayout pipeline_layout {};
        vk::DescriptorPool descriptor_pool {};
        vk::Pipeline pipeline {};
    };
}
