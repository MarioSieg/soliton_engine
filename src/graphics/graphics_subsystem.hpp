// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "vulkancore/context.hpp"
#include "vulkancore/shader.hpp"
#include "vulkancore/buffer.hpp"

#include "mesh.hpp"
#include "texture.hpp"

namespace graphics {
    struct uniform_buffer final {
        vkb::buffer buffer {};
        vk::DescriptorSet descriptor_set {};
    };

    struct gpu_vertex_push_constants final {
        DirectX::XMMATRIX model_view_proj;
        DirectX::XMMATRIX normal_matrix;
    };
    static_assert(sizeof(gpu_vertex_push_constants) <= 128);

    struct gpu_uniform_buffer final {
        DirectX::XMMATRIX model_view_proj;
        DirectX::XMMATRIX normal_matrix;
    };
    static_assert(sizeof(gpu_uniform_buffer) % 4 == 0);

    class graphics_subsystem final : public subsystem {
    public:
        static constexpr std::uint32_t k_max_concurrent_frames = 3;
        graphics_subsystem();
        ~graphics_subsystem() override;

        HOTPROC auto on_pre_tick() -> bool override;
        HOTPROC auto on_post_tick() -> void override;
        auto on_resize() -> void override;
        auto on_start(scene& scene) -> void override;

        [[nodiscard]] static auto get_descriptor_pool() noexcept -> vk::DescriptorPool {
            return m_descriptor_pool;
        }

    private:
        auto create_uniform_buffers() -> void;
        auto create_descriptor_set_layout() -> void;
        auto create_descriptor_pool(std::uint32_t num_resources) const -> void;
        auto create_descriptor_sets() -> void;
        auto create_pipeline() -> void;

        auto render_scene(vk::CommandBuffer cmd_buf) -> void;

        vk::CommandBuffer m_cmd_buf = nullptr;
        std::array<uniform_buffer, vkb::context::k_max_concurrent_frames> m_uniforms {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
        vk::PipelineLayout m_pipeline_layout {};
        static inline constinit vk::DescriptorPool m_descriptor_pool {};
        vk::Pipeline m_pipeline {};

        struct {
            flecs::query<const c_transform, const c_mesh_renderer> query {};
            scene* scene {};
        } m_render_query {};

        std::optional<texture> m_texture {};
    };
}
