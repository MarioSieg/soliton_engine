// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <optional>

#include "../core/subsystem.hpp"
#include "../scene/scene.hpp"

#include "vulkancore/context.hpp"
#include "vulkancore/shader.hpp"
#include "vulkancore/buffer.hpp"

#include "render_thread_pool.hpp"

#include "mesh.hpp"
#include "texture.hpp"

namespace graphics {
    struct uniform_buffer final {
        vkb::buffer buffer {};
        vk::DescriptorSet descriptor_set {};
    };

    struct gpu_vertex_push_constants final {
        DirectX::XMFLOAT4X4A model_view_proj;
        DirectX::XMFLOAT4X4A normal_matrix;
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

        [[nodiscard]] auto get_pipeline_layout() const noexcept -> vk::PipelineLayout { return m_pipeline_layout; }
        [[nodiscard]] auto get_pipeline() const noexcept -> vk::Pipeline { return m_pipeline; }
        [[nodiscard]] auto get_descriptor_set_layout() const noexcept -> vk::DescriptorSetLayout { return m_descriptor_set_layout; }
        [[nodiscard]] auto get_descriptor_pool() const noexcept -> vk::DescriptorPool { return m_descriptor_pool; }
        [[nodiscard]] auto get_render_thread_pool() const noexcept -> const render_thread_pool& { return *m_render_thread_pool; }

        [[nodiscard]] auto get_uniforms() const noexcept -> std::span<const uniform_buffer> { return m_uniforms; }
        [[nodiscard]] auto get_transforms() const noexcept -> std::span<const c_transform> { return m_transforms; }
        [[nodiscard]] auto get_renderers() const noexcept -> std::span<const c_mesh_renderer> { return m_renderers; }

    private:
        auto create_uniform_buffers() -> void;
        auto create_descriptor_set_layout() -> void;
        auto create_descriptor_pool() -> void;
        auto create_descriptor_sets() -> void;
        auto create_pipeline() -> void;

        vk::CommandBuffer m_cmd_buf = nullptr;
        std::array<uniform_buffer, vkb::context::k_max_concurrent_frames> m_uniforms {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
        vk::PipelineLayout m_pipeline_layout {};
        vk::DescriptorPool m_descriptor_pool {};
        vk::Pipeline m_pipeline {};
        vk::CommandBufferInheritanceInfo m_inheritance_info {};
        std::optional<render_thread_pool> m_render_thread_pool {};
        std::span<const c_transform> m_transforms {};
        std::span<const c_mesh_renderer> m_renderers {};

        struct {
            flecs::query<const c_transform, const c_mesh_renderer> query {};
            scene* scene {};
        } m_render_query {};
    };
}
