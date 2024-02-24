// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include "vulkancore/prelude.hpp"
#include "vulkancore/buffer.hpp"

namespace graphics {
    class debugdraw final : public no_copy, public no_move {
    public:
        struct uniform final {
            DirectX::XMFLOAT4X4A view_proj {};
        };

        struct vertex final {
            DirectX::XMFLOAT3 pos {};
            DirectX::XMFLOAT2 uv {};
            DirectX::XMFLOAT3 color {};
        };

        struct draw_command final {
            bool is_line_list = true;
            std::uint32_t num_vertices = 0;
            bool depth_test = false;
            bool distance_fade = false;
            float fade_start = 0.0f;
            float fade_end = 0.0f;
        };

        explicit debugdraw(vk::DescriptorPool pool);
        ~debugdraw();

        // fuck me
        auto set_depth_test(const bool enable) noexcept -> void { m_depth_test = enable; }
        [[nodiscard]] auto is_deph_test_enabled() const noexcept -> bool { return m_depth_test; }
        auto set_distance_fade_enable(const bool enable) noexcept -> void { m_distance_fade = enable; }
        [[nodiscard]] auto is_distance_face_enabled() const noexcept -> bool { return m_distance_fade; }
        auto set_fade_start(const float x) noexcept -> void { m_fade_start = x; }
        [[nodiscard]] auto get_fade_start() const noexcept -> float { return m_fade_start; }
        auto set_fade_end(const float x) noexcept -> void { m_fade_end = x; }
        [[nodiscard]] auto get_fade_end() const noexcept -> float { return m_fade_end; }
        auto begin_batch() noexcept -> void {
            m_batched_mode = true;
            m_batch_start = m_vertices.size();
            m_batch_end = m_batch_start;
        }
        auto end_batch() noexcept -> void {
            m_batched_mode = false;
        }

        auto draw_line(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& color) -> void;
        auto draw_grid(const DirectX::XMFLOAT3& pos, float step, const DirectX::XMFLOAT3& color) -> void;
        auto draw_aabb(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max, const DirectX::XMFLOAT3& color) -> void;
        auto draw_aabb(const DirectX::BoundingBox& aabb, const DirectX::XMFLOAT3& color) -> void;
        auto draw_obb(const DirectX::BoundingOrientedBox& obb, DirectX::FXMMATRIX model, const DirectX::XMFLOAT3& color) -> void;
        auto draw_transform(DirectX::FXMMATRIX mtx, float axis_len) -> void;
        auto render(vk::CommandBuffer cmd, DirectX::FXMMATRIX view_proj, DirectX::FXMVECTOR view_pos) -> void;

    private:
        auto create_descriptor_set_layout(vk::Device device) -> void;
        auto create_descriptor_set(vk::Device device, vk::DescriptorPool pool) -> void;
        auto create_uniform_buffer() -> void;
        auto create_vertex_buffer() -> void;
        auto create_pipeline_states(vk::Device device, vk::RenderPass pass) -> void;

        const std::uint32_t k_max_vertices;
        std::vector<vertex> m_vertices {};
        std::vector<draw_command> m_draw_commands {};
        bool m_depth_test = false;
        std::uint32_t m_batch_start = 0;
        std::uint32_t m_batch_end = 0;
        bool m_batched_mode = false;
        float m_fade_start = 0.0f;
        float m_fade_end = 0.0f;
        bool m_distance_fade = false;
        vkb::buffer m_vertex_buffer {};
        vkb::buffer m_uniform {};
        vk::PipelineLayout m_pipeline_layout {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
        vk::DescriptorSet m_descriptor_set {};
        vk::Pipeline m_line_depth_pipeline {};
        vk::Pipeline m_line_no_depth_pipeline {};
        vk::Pipeline m_line_strip_depth_pipeline {};
        vk::Pipeline m_line_strip_no_depth_pipeline {};
    };
}
