// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "debugdraw.hpp"

#include "../../core/system_variable.hpp"
#include "../vulkancore/context.hpp"

namespace soliton::graphics {
    static const system_variable<std::int64_t> k_debug_draw_max_verts {
        "renderer.max_debug_draw_vertices",
        {0x20000}
    };

    using namespace DirectX;

    static constexpr eastl::array<const std::uint32_t, 1532 / 4> k_debug_draw_vs_spirv {
        0x07230203,
        0x00010000,
        0x000d0008,
        0x0000002c,
        0x00000000,
        0x00020011,
        0x00000001,
        0x0006000b,
        0x00000001,
        0x4c534c47,
        0x6474732e,
        0x3035342e,
        0x00000000,
        0x0003000e,
        0x00000000,
        0x00000001,
        0x000b000f,
        0x00000000,
        0x00000004,
        0x6e69616d,
        0x00000000,
        0x00000009,
        0x0000000b,
        0x0000000d,
        0x0000000e,
        0x00000016,
        0x0000002b,
        0x00030003,
        0x00000002,
        0x000001c2,
        0x000a0004,
        0x475f4c47,
        0x4c474f4f,
        0x70635f45,
        0x74735f70,
        0x5f656c79,
        0x656e696c,
        0x7269645f,
        0x69746365,
        0x00006576,
        0x00080004,
        0x475f4c47,
        0x4c474f4f,
        0x6e695f45,
        0x64756c63,
        0x69645f65,
        0x74636572,
        0x00657669,
        0x00040005,
        0x00000004,
        0x6e69616d,
        0x00000000,
        0x00050005,
        0x00000009,
        0x495f5346,
        0x6f435f4e,
        0x00726f6c,
        0x00050005,
        0x0000000b,
        0x495f5356,
        0x6f435f4e,
        0x00726f6c,
        0x00060005,
        0x0000000d,
        0x495f5346,
        0x72465f4e,
        0x6f506761,
        0x00000073,
        0x00060005,
        0x0000000e,
        0x495f5356,
        0x6f505f4e,
        0x69746973,
        0x00006e6f,
        0x00060005,
        0x00000014,
        0x505f6c67,
        0x65567265,
        0x78657472,
        0x00000000,
        0x00060006,
        0x00000014,
        0x00000000,
        0x505f6c67,
        0x7469736f,
        0x006e6f69,
        0x00070006,
        0x00000014,
        0x00000001,
        0x505f6c67,
        0x746e696f,
        0x657a6953,
        0x00000000,
        0x00070006,
        0x00000014,
        0x00000002,
        0x435f6c67,
        0x4470696c,
        0x61747369,
        0x0065636e,
        0x00070006,
        0x00000014,
        0x00000003,
        0x435f6c67,
        0x446c6c75,
        0x61747369,
        0x0065636e,
        0x00030005,
        0x00000016,
        0x00000000,
        0x00060005,
        0x0000001a,
        0x656d6143,
        0x6e556172,
        0x726f6669,
        0x0000736d,
        0x00060006,
        0x0000001a,
        0x00000000,
        0x77656976,
        0x6a6f7250,
        0x00000000,
        0x00030005,
        0x0000001c,
        0x00000000,
        0x00060005,
        0x0000002b,
        0x495f5356,
        0x65545f4e,
        0x6f6f4378,
        0x00006472,
        0x00040047,
        0x00000009,
        0x0000001e,
        0x00000000,
        0x00040047,
        0x0000000b,
        0x0000001e,
        0x00000002,
        0x00040047,
        0x0000000d,
        0x0000001e,
        0x00000001,
        0x00040047,
        0x0000000e,
        0x0000001e,
        0x00000000,
        0x00050048,
        0x00000014,
        0x00000000,
        0x0000000b,
        0x00000000,
        0x00050048,
        0x00000014,
        0x00000001,
        0x0000000b,
        0x00000001,
        0x00050048,
        0x00000014,
        0x00000002,
        0x0000000b,
        0x00000003,
        0x00050048,
        0x00000014,
        0x00000003,
        0x0000000b,
        0x00000004,
        0x00030047,
        0x00000014,
        0x00000002,
        0x00040048,
        0x0000001a,
        0x00000000,
        0x00000005,
        0x00050048,
        0x0000001a,
        0x00000000,
        0x00000023,
        0x00000000,
        0x00050048,
        0x0000001a,
        0x00000000,
        0x00000007,
        0x00000010,
        0x00030047,
        0x0000001a,
        0x00000002,
        0x00040047,
        0x0000001c,
        0x00000022,
        0x00000000,
        0x00040047,
        0x0000001c,
        0x00000021,
        0x00000000,
        0x00040047,
        0x0000002b,
        0x0000001e,
        0x00000001,
        0x00020013,
        0x00000002,
        0x00030021,
        0x00000003,
        0x00000002,
        0x00030016,
        0x00000006,
        0x00000020,
        0x00040017,
        0x00000007,
        0x00000006,
        0x00000003,
        0x00040020,
        0x00000008,
        0x00000003,
        0x00000007,
        0x0004003b,
        0x00000008,
        0x00000009,
        0x00000003,
        0x00040020,
        0x0000000a,
        0x00000001,
        0x00000007,
        0x0004003b,
        0x0000000a,
        0x0000000b,
        0x00000001,
        0x0004003b,
        0x00000008,
        0x0000000d,
        0x00000003,
        0x0004003b,
        0x0000000a,
        0x0000000e,
        0x00000001,
        0x00040017,
        0x00000010,
        0x00000006,
        0x00000004,
        0x00040015,
        0x00000011,
        0x00000020,
        0x00000000,
        0x0004002b,
        0x00000011,
        0x00000012,
        0x00000001,
        0x0004001c,
        0x00000013,
        0x00000006,
        0x00000012,
        0x0006001e,
        0x00000014,
        0x00000010,
        0x00000006,
        0x00000013,
        0x00000013,
        0x00040020,
        0x00000015,
        0x00000003,
        0x00000014,
        0x0004003b,
        0x00000015,
        0x00000016,
        0x00000003,
        0x00040015,
        0x00000017,
        0x00000020,
        0x00000001,
        0x0004002b,
        0x00000017,
        0x00000018,
        0x00000000,
        0x00040018,
        0x00000019,
        0x00000010,
        0x00000004,
        0x0003001e,
        0x0000001a,
        0x00000019,
        0x00040020,
        0x0000001b,
        0x00000002,
        0x0000001a,
        0x0004003b,
        0x0000001b,
        0x0000001c,
        0x00000002,
        0x00040020,
        0x0000001d,
        0x00000002,
        0x00000019,
        0x0004002b,
        0x00000006,
        0x00000021,
        0x3f800000,
        0x00040020,
        0x00000027,
        0x00000003,
        0x00000010,
        0x00040017,
        0x00000029,
        0x00000006,
        0x00000002,
        0x00040020,
        0x0000002a,
        0x00000001,
        0x00000029,
        0x0004003b,
        0x0000002a,
        0x0000002b,
        0x00000001,
        0x00050036,
        0x00000002,
        0x00000004,
        0x00000000,
        0x00000003,
        0x000200f8,
        0x00000005,
        0x0004003d,
        0x00000007,
        0x0000000c,
        0x0000000b,
        0x0003003e,
        0x00000009,
        0x0000000c,
        0x0004003d,
        0x00000007,
        0x0000000f,
        0x0000000e,
        0x0003003e,
        0x0000000d,
        0x0000000f,
        0x00050041,
        0x0000001d,
        0x0000001e,
        0x0000001c,
        0x00000018,
        0x0004003d,
        0x00000019,
        0x0000001f,
        0x0000001e,
        0x0004003d,
        0x00000007,
        0x00000020,
        0x0000000e,
        0x00050051,
        0x00000006,
        0x00000022,
        0x00000020,
        0x00000000,
        0x00050051,
        0x00000006,
        0x00000023,
        0x00000020,
        0x00000001,
        0x00050051,
        0x00000006,
        0x00000024,
        0x00000020,
        0x00000002,
        0x00070050,
        0x00000010,
        0x00000025,
        0x00000022,
        0x00000023,
        0x00000024,
        0x00000021,
        0x00050091,
        0x00000010,
        0x00000026,
        0x0000001f,
        0x00000025,
        0x00050041,
        0x00000027,
        0x00000028,
        0x00000016,
        0x00000018,
        0x0003003e,
        0x00000028,
        0x00000026,
        0x000100fd,
        0x00010038,
    };

    static constexpr eastl::array<const std::uint32_t, 1904 / 4> k_debug_draw_fs_spirv {
        0x07230203,
        0x00010000,
        0x000d0008,
        0x00000047,
        0x00000000,
        0x00020011,
        0x00000001,
        0x0006000b,
        0x00000001,
        0x4c534c47,
        0x6474732e,
        0x3035342e,
        0x00000000,
        0x0003000e,
        0x00000000,
        0x00000001,
        0x0008000f,
        0x00000004,
        0x00000004,
        0x6e69616d,
        0x00000000,
        0x0000001f,
        0x00000022,
        0x0000002b,
        0x00030010,
        0x00000004,
        0x00000007,
        0x00030003,
        0x00000002,
        0x000001c2,
        0x000a0004,
        0x475f4c47,
        0x4c474f4f,
        0x70635f45,
        0x74735f70,
        0x5f656c79,
        0x656e696c,
        0x7269645f,
        0x69746365,
        0x00006576,
        0x00080004,
        0x475f4c47,
        0x4c474f4f,
        0x6e695f45,
        0x64756c63,
        0x69645f65,
        0x74636572,
        0x00657669,
        0x00040005,
        0x00000004,
        0x6e69616d,
        0x00000000,
        0x00050005,
        0x00000008,
        0x65646166,
        0x6174735f,
        0x00007472,
        0x00060005,
        0x0000000a,
        0x68737550,
        0x736e6f43,
        0x746e6174,
        0x00000073,
        0x00060006,
        0x0000000a,
        0x00000000,
        0x656d6163,
        0x705f6172,
        0x0000736f,
        0x00060006,
        0x0000000a,
        0x00000001,
        0x65646166,
        0x7261705f,
        0x00736d61,
        0x00050005,
        0x0000000c,
        0x68737570,
        0x6e6f635f,
        0x00737473,
        0x00050005,
        0x00000014,
        0x65646166,
        0x646e655f,
        0x00000000,
        0x00060005,
        0x0000001f,
        0x4f5f5346,
        0x435f5455,
        0x726f6c6f,
        0x00000000,
        0x00050005,
        0x00000022,
        0x495f5346,
        0x6f435f4e,
        0x00726f6c,
        0x00050005,
        0x0000002a,
        0x74736964,
        0x65636e61,
        0x00000000,
        0x00060005,
        0x0000002b,
        0x495f5346,
        0x72465f4e,
        0x6f506761,
        0x00000073,
        0x00070005,
        0x00000034,
        0x65746661,
        0x61665f72,
        0x735f6564,
        0x74726174,
        0x00000000,
        0x00040005,
        0x00000039,
        0x6361706f,
        0x00797469,
        0x00050048,
        0x0000000a,
        0x00000000,
        0x00000023,
        0x00000000,
        0x00050048,
        0x0000000a,
        0x00000001,
        0x00000023,
        0x00000010,
        0x00030047,
        0x0000000a,
        0x00000002,
        0x00040047,
        0x0000001f,
        0x0000001e,
        0x00000000,
        0x00040047,
        0x00000022,
        0x0000001e,
        0x00000000,
        0x00040047,
        0x0000002b,
        0x0000001e,
        0x00000001,
        0x00020013,
        0x00000002,
        0x00030021,
        0x00000003,
        0x00000002,
        0x00030016,
        0x00000006,
        0x00000020,
        0x00040020,
        0x00000007,
        0x00000007,
        0x00000006,
        0x00040017,
        0x00000009,
        0x00000006,
        0x00000004,
        0x0004001e,
        0x0000000a,
        0x00000009,
        0x00000009,
        0x00040020,
        0x0000000b,
        0x00000009,
        0x0000000a,
        0x0004003b,
        0x0000000b,
        0x0000000c,
        0x00000009,
        0x00040015,
        0x0000000d,
        0x00000020,
        0x00000001,
        0x0004002b,
        0x0000000d,
        0x0000000e,
        0x00000001,
        0x00040015,
        0x0000000f,
        0x00000020,
        0x00000000,
        0x0004002b,
        0x0000000f,
        0x00000010,
        0x00000000,
        0x00040020,
        0x00000011,
        0x00000009,
        0x00000006,
        0x0004002b,
        0x0000000f,
        0x00000015,
        0x00000001,
        0x0004002b,
        0x00000006,
        0x00000019,
        0x00000000,
        0x00020014,
        0x0000001a,
        0x00040020,
        0x0000001e,
        0x00000003,
        0x00000009,
        0x0004003b,
        0x0000001e,
        0x0000001f,
        0x00000003,
        0x00040017,
        0x00000020,
        0x00000006,
        0x00000003,
        0x00040020,
        0x00000021,
        0x00000001,
        0x00000020,
        0x0004003b,
        0x00000021,
        0x00000022,
        0x00000001,
        0x0004002b,
        0x00000006,
        0x00000024,
        0x3f800000,
        0x0004003b,
        0x00000021,
        0x0000002b,
        0x00000001,
        0x0004002b,
        0x0000000d,
        0x0000002d,
        0x00000000,
        0x00040020,
        0x0000002e,
        0x00000009,
        0x00000009,
        0x00050036,
        0x00000002,
        0x00000004,
        0x00000000,
        0x00000003,
        0x000200f8,
        0x00000005,
        0x0004003b,
        0x00000007,
        0x00000008,
        0x00000007,
        0x0004003b,
        0x00000007,
        0x00000014,
        0x00000007,
        0x0004003b,
        0x00000007,
        0x0000002a,
        0x00000007,
        0x0004003b,
        0x00000007,
        0x00000034,
        0x00000007,
        0x0004003b,
        0x00000007,
        0x00000039,
        0x00000007,
        0x00060041,
        0x00000011,
        0x00000012,
        0x0000000c,
        0x0000000e,
        0x00000010,
        0x0004003d,
        0x00000006,
        0x00000013,
        0x00000012,
        0x0003003e,
        0x00000008,
        0x00000013,
        0x00060041,
        0x00000011,
        0x00000016,
        0x0000000c,
        0x0000000e,
        0x00000015,
        0x0004003d,
        0x00000006,
        0x00000017,
        0x00000016,
        0x0003003e,
        0x00000014,
        0x00000017,
        0x0004003d,
        0x00000006,
        0x00000018,
        0x00000008,
        0x000500b8,
        0x0000001a,
        0x0000001b,
        0x00000018,
        0x00000019,
        0x000300f7,
        0x0000001d,
        0x00000000,
        0x000400fa,
        0x0000001b,
        0x0000001c,
        0x00000029,
        0x000200f8,
        0x0000001c,
        0x0004003d,
        0x00000020,
        0x00000023,
        0x00000022,
        0x00050051,
        0x00000006,
        0x00000025,
        0x00000023,
        0x00000000,
        0x00050051,
        0x00000006,
        0x00000026,
        0x00000023,
        0x00000001,
        0x00050051,
        0x00000006,
        0x00000027,
        0x00000023,
        0x00000002,
        0x00070050,
        0x00000009,
        0x00000028,
        0x00000025,
        0x00000026,
        0x00000027,
        0x00000024,
        0x0003003e,
        0x0000001f,
        0x00000028,
        0x000200f9,
        0x0000001d,
        0x000200f8,
        0x00000029,
        0x0004003d,
        0x00000020,
        0x0000002c,
        0x0000002b,
        0x00050041,
        0x0000002e,
        0x0000002f,
        0x0000000c,
        0x0000002d,
        0x0004003d,
        0x00000009,
        0x00000030,
        0x0000002f,
        0x0008004f,
        0x00000020,
        0x00000031,
        0x00000030,
        0x00000030,
        0x00000000,
        0x00000001,
        0x00000002,
        0x00050083,
        0x00000020,
        0x00000032,
        0x0000002c,
        0x00000031,
        0x0006000c,
        0x00000006,
        0x00000033,
        0x00000001,
        0x00000042,
        0x00000032,
        0x0003003e,
        0x0000002a,
        0x00000033,
        0x0004003d,
        0x00000006,
        0x00000035,
        0x0000002a,
        0x0004003d,
        0x00000006,
        0x00000036,
        0x00000008,
        0x00050083,
        0x00000006,
        0x00000037,
        0x00000035,
        0x00000036,
        0x0007000c,
        0x00000006,
        0x00000038,
        0x00000001,
        0x00000028,
        0x00000037,
        0x00000019,
        0x0003003e,
        0x00000034,
        0x00000038,
        0x0004003d,
        0x00000006,
        0x0000003a,
        0x00000034,
        0x0004003d,
        0x00000006,
        0x0000003b,
        0x00000014,
        0x0004003d,
        0x00000006,
        0x0000003c,
        0x00000008,
        0x00050083,
        0x00000006,
        0x0000003d,
        0x0000003b,
        0x0000003c,
        0x00050088,
        0x00000006,
        0x0000003e,
        0x0000003a,
        0x0000003d,
        0x00050083,
        0x00000006,
        0x0000003f,
        0x00000024,
        0x0000003e,
        0x0008000c,
        0x00000006,
        0x00000040,
        0x00000001,
        0x0000002b,
        0x0000003f,
        0x00000019,
        0x00000024,
        0x0003003e,
        0x00000039,
        0x00000040,
        0x0004003d,
        0x00000020,
        0x00000041,
        0x00000022,
        0x0004003d,
        0x00000006,
        0x00000042,
        0x00000039,
        0x00050051,
        0x00000006,
        0x00000043,
        0x00000041,
        0x00000000,
        0x00050051,
        0x00000006,
        0x00000044,
        0x00000041,
        0x00000001,
        0x00050051,
        0x00000006,
        0x00000045,
        0x00000041,
        0x00000002,
        0x00070050,
        0x00000009,
        0x00000046,
        0x00000043,
        0x00000044,
        0x00000045,
        0x00000042,
        0x0003003e,
        0x0000001f,
        0x00000046,
        0x000200f9,
        0x0000001d,
        0x000200f8,
        0x0000001d,
        0x000100fd,
        0x00010038,
    };

    auto debugdraw::render(
        const vk::CommandBuffer cmd,
        FXMMATRIX view_proj,
        FXMVECTOR view_pos
    ) -> void {
        if (!m_vertices.empty()) {
            const std::uint32_t frameidx = vkb::ctx().get_current_concurrent_frame_idx();
            uniform_data uniform_data {};
            XMStoreFloat4x4A(&uniform_data.view_proj, view_proj);
            m_uniform->set(uniform_data);
            if (m_vertices.size() > k_max_vertices) {
                log_error("Too many vertices: {}, max: {}", m_vertices.size(), k_max_vertices);
            } else {
                auto* vptr = static_cast<std::uint8_t*>(m_vertex_buffer->get_mapped_ptr());
                std::memcpy(vptr + sizeof(vertex) * k_max_vertices * frameidx, m_vertices.data(), sizeof(vertex) * m_vertices.size());
            }
            std::uint32_t dynamic_offset = m_uniform->get_dynamic_aligned_size() * frameidx;
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, 1, &m_descriptor_set, 1, &dynamic_offset);
            const vk::DeviceSize vbo_offset = sizeof(vertex) * k_max_vertices * frameidx;
            cmd.bindVertexBuffers(0, 1, &m_vertex_buffer->get_buffer(), &vbo_offset);
            for (std::size_t v = 0; const draw_command& draw_cmd : m_draw_commands) {
                vk::Pipeline pipe {};
                if (draw_cmd.is_line_list) {
                    pipe = draw_cmd.depth_test ? m_line_depth_pipeline : m_line_no_depth_pipeline;
                } else {
                    pipe = draw_cmd.depth_test ? m_line_strip_depth_pipeline : m_line_strip_no_depth_pipeline;
                }
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe);
                eastl::array<XMFLOAT4A, 2> params {};
                static_assert(sizeof(params) == sizeof(float) * 4 * 2);
                XMStoreFloat4A(&params[0], view_pos);
                XMStoreFloat4A(&params[1], XMVectorSet(draw_cmd.distance_fade ? draw_cmd.fade_start : -1.0f, draw_cmd.fade_end, 0.0f, 0.0f));
                cmd.pushConstants(m_pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(params), params.data());
                cmd.draw(draw_cmd.num_vertices, 1, v, 0);
                v += draw_cmd.num_vertices;
            }
        }
        m_vertices.clear();
        m_draw_commands.clear();
    }

    auto debugdraw::draw_line(
        const XMFLOAT3& from,
        const XMFLOAT3& to,
        const XMFLOAT3& color
    ) -> void {
        vertex vw0 {}, vw1 {};
        vw0.pos = from;
        vw0.color = color;
        vw1.pos = to;
        vw1.color = color;
        m_vertices.emplace_back(vw0);
        m_vertices.emplace_back(vw1);
        if (!m_batched_mode || m_batch_start == m_batch_end) {
            draw_command cmd {};
            cmd.depth_test = m_depth_test;
            cmd.distance_fade = m_distance_fade;
            cmd.fade_start = m_fade_start;
            cmd.fade_end = m_fade_end;
            cmd.is_line_list = true;
            cmd.num_vertices = 2;
            m_draw_commands.emplace_back(cmd);
        } else {
            draw_command& cmd = m_draw_commands.back();
            cmd.num_vertices += 2;
        }
        if (m_batched_mode) {
            m_batch_end = m_vertices.size();
        }
    }

    auto debugdraw::draw_arrow(const XMFLOAT3& from, const XMFLOAT3& to, const XMFLOAT3& color, const float arrowhead_length) -> void {
        draw_line(from, to, color);
        XMVECTOR fromVec = XMLoadFloat3(&from);
        XMVECTOR toVec = XMLoadFloat3(&to);
        XMVECTOR dir = XMVectorSubtract(toVec, fromVec);
        XMVECTOR dirNormalized = XMVector3Normalize(dir);
        XMVECTOR arrowheadDir = XMVectorScale(dirNormalized, arrowhead_length);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR perp1 = XMVector3Cross(dirNormalized, up);
        perp1 = XMVector3Normalize(perp1);
        float angle_step = XM_PIDIV4;  // 45 degrees in radians
        for (int i = 0; i < 16; ++i) {
            XMVECTOR rotation = XMQuaternionRotationAxis(dirNormalized, angle_step * static_cast<float>(i));
            XMVECTOR rotatedPerp = XMVector3Rotate(perp1, rotation);
            XMVECTOR perpScaled = XMVectorScale(rotatedPerp, 0.1f);  // Adjust width of the arrowhead
            XMVECTOR arrowheadPoint = XMVectorSubtract(toVec, arrowheadDir);
            arrowheadPoint = XMVectorAdd(arrowheadPoint, perpScaled);
            XMFLOAT3 arrowheadPointFloat;
            XMStoreFloat3(&arrowheadPointFloat, arrowheadPoint);
            draw_line(to, arrowheadPointFloat, color);
        }
    }

    auto debugdraw::draw_arrow_dir(const XMFLOAT3& from, const XMFLOAT3& dir, const XMFLOAT3& color, const float arrowhead_length) -> void {
        XMVECTOR fromVec = XMLoadFloat3(&from);
        XMVECTOR dirVec = XMLoadFloat3(&dir);
        XMVECTOR toVec = XMVectorAdd(fromVec, dirVec);
        XMFLOAT3 to;
        XMStoreFloat3(&to, toVec);
        draw_arrow(from, to, color, arrowhead_length);
    }

    auto debugdraw::draw_grid(const XMFLOAT3& pos, const float step, const XMFLOAT3& color) -> void {
        begin_batch();
        const float offset_x = std::floor(pos.x * step / 2.0f);
        const float offset_z = std::floor(pos.z * step / 2.0f);
        for (int x = static_cast<int>(-offset_x); x <= static_cast<int>(offset_x); x += static_cast<int>(step)) {
            draw_line(XMFLOAT3{static_cast<float>(x), pos.y, -offset_z}, XMFLOAT3{static_cast<float>(x), pos.y, offset_z}, color);
        }
        for (int z = static_cast<int>(-offset_z); z <= static_cast<int>(offset_z); z += static_cast<int>(step)) {
            draw_line(XMFLOAT3{-offset_x, pos.y, static_cast<float>(z)}, XMFLOAT3{offset_x, pos.y, static_cast<float>(z)}, color);
        }
        end_batch();
    }

    auto debugdraw::draw_aabb(
        const XMFLOAT3& min,
        const XMFLOAT3& max,
        const XMFLOAT3& color
    ) -> void {
        begin_batch();
        draw_line(min, XMFLOAT3(max.x, min.y, min.z), color);
        draw_line(XMFLOAT3(max.x, min.y, min.z), XMFLOAT3(max.x, min.y, max.z), color);
        draw_line(XMFLOAT3(max.x, min.y, max.z), XMFLOAT3(min.x, min.y, max.z), color);
        draw_line(XMFLOAT3(min.x, min.y, max.z), min, color);
        draw_line(XMFLOAT3(min.x, max.y, min.z), XMFLOAT3(max.x, max.y, min.z), color);
        draw_line(XMFLOAT3(max.x, max.y, min.z), max, color);
        draw_line(max, XMFLOAT3(min.x, max.y, max.z), color);
        draw_line(XMFLOAT3(min.x, max.y, max.z), XMFLOAT3(min.x, max.y, min.z), color);
        draw_line(min, XMFLOAT3(min.x, max.y, min.z), color);
        draw_line(XMFLOAT3(max.x, min.y, min.z), XMFLOAT3(max.x, max.y, min.z), color);
        draw_line(XMFLOAT3(max.x, min.y, max.z), max, color);
        draw_line(XMFLOAT3(min.x, min.y, max.z), XMFLOAT3(min.x, max.y, max.z), color);
        end_batch();
    }

    auto debugdraw::draw_aabb(const BoundingBox& aabb, const XMFLOAT3& color) -> void {
        XMFLOAT3A min, max;
        XMStoreFloat3A(&min, XMVectorSubtract(XMLoadFloat3(&aabb.Center), XMLoadFloat3(&aabb.Extents)));
        XMStoreFloat3A(&max, XMVectorAdd(XMLoadFloat3(&aabb.Center), XMLoadFloat3(&aabb.Extents)));
        draw_aabb(min, max, color);
    }

    auto debugdraw::draw_obb(const BoundingOrientedBox& obb, FXMMATRIX model, const XMFLOAT3& color) -> void {
        eastl::array<XMFLOAT3, 8> verts {};
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    XMFLOAT3 corner = {
                        obb.Center.x + (i == 0 ? -obb.Extents.x : obb.Extents.x),
                        obb.Center.y + (j == 0 ? -obb.Extents.y : obb.Extents.y),
                        obb.Center.z + (k == 0 ? -obb.Extents.z : obb.Extents.z)
                    };
                    XMVECTOR cornerVec = XMVector3Transform(XMLoadFloat3(&corner), model);
                   XMStoreFloat3(&verts[i * 4 + j * 2 + k], cornerVec);
                }
            }
        }
        draw_line(verts[0], verts[1], color);
        draw_line(verts[1], verts[3], color);
        draw_line(verts[3], verts[2], color);
        draw_line(verts[2], verts[0], color);
        draw_line(verts[4], verts[5], color);
        draw_line(verts[5], verts[7], color);
        draw_line(verts[7], verts[6], color);
        draw_line(verts[6], verts[4], color);
        draw_line(verts[0], verts[4], color);
        draw_line(verts[1], verts[5], color);
        draw_line(verts[2], verts[6], color);
        draw_line(verts[3], verts[7], color);
    }

    auto debugdraw::draw_transform(FXMMATRIX mtx, const float axis_len) -> void {
        XMVECTOR scale, rotation, translation;
        XMMatrixDecompose(&scale, &rotation, &translation, mtx);
        XMFLOAT3A p;
        XMStoreFloat3A(&p, translation);
        XMVECTOR x_dir = XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
        XMVECTOR y_dir = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);
        XMVECTOR z_dir = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        XMFLOAT3A x_end, y_end, z_end;
        XMStoreFloat3A(&x_end, XMVectorAdd(XMLoadFloat3(&p), XMVectorScale(x_dir, axis_len)));
        XMStoreFloat3A(&y_end, XMVectorAdd(XMLoadFloat3(&p), XMVectorScale(y_dir, axis_len)));
        XMStoreFloat3A(&z_end, XMVectorAdd(XMLoadFloat3(&p), XMVectorScale(z_dir, axis_len)));
        draw_line(p, x_end, XMFLOAT3(1.0f, 0.0f, 0.0f));
        draw_line(p, y_end, XMFLOAT3(0.0f, 1.0f, 0.0f));
        draw_line(p, z_end, XMFLOAT3(0.0f, 0.0f, 1.0f));
    }

    auto debugdraw::create_descriptor() -> void {
        vkb::descriptor_factory factory {vkb::ctx().get_descriptor_layout_cache(), vkb::ctx().get_descriptor_allocator()};
        vk::DescriptorBufferInfo buffer_info {};
        buffer_info.buffer = m_uniform->get_buffer();
        buffer_info.offset = 0;
        buffer_info.range = m_uniform->get_dynamic_aligned_size();
        factory.bind_buffers(0, 1, &buffer_info, vk::DescriptorType::eUniformBufferDynamic, vk::ShaderStageFlagBits::eVertex);
        panic_assert(factory.build(m_descriptor_set, m_descriptor_set_layout));
    }

    auto debugdraw::create_uniform_buffer() -> void {
        m_uniform.emplace();
    }

    auto debugdraw::create_vertex_buffer() -> void {
        m_vertex_buffer.emplace(
           sizeof(vertex) * k_max_vertices * vkb::ctx().get_concurrent_frame_count(),
           0,
           vk::BufferUsageFlagBits::eVertexBuffer,
           VMA_MEMORY_USAGE_CPU_TO_GPU,
           VMA_ALLOCATION_CREATE_MAPPED_BIT
       );
    }

    auto debugdraw::create_pipeline_states(const vk::Device device, const vk::RenderPass pass) -> void {
        vk::ShaderModuleCreateInfo shader_create_info {};

        shader_create_info.codeSize = k_debug_draw_vs_spirv.size() * sizeof(std::uint32_t);
        shader_create_info.pCode = k_debug_draw_vs_spirv.data();

        vk::ShaderModule vs;
        vkcheck(device.createShaderModule(&shader_create_info, vkb::get_alloc(), &vs));

        shader_create_info.codeSize = k_debug_draw_fs_spirv.size() * sizeof(std::uint32_t);
        shader_create_info.pCode = k_debug_draw_fs_spirv.data();
        vk::ShaderModule fs;
        vkcheck(device.createShaderModule(&shader_create_info, vkb::get_alloc(), &fs));

        vk::GraphicsPipelineCreateInfo pipeline_info {};

        eastl::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {};
        shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
        shader_stages[0].module = vs;
        shader_stages[0].pName = "main";
        shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
        shader_stages[1].module = fs;
        shader_stages[1].pName = "main";
        pipeline_info.stageCount = shader_stages.size();
        pipeline_info.pStages = shader_stages.data();

        vk::PipelineViewportStateCreateInfo viewport_state {};
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;
        pipeline_info.pViewportState = &viewport_state;

        vk::PipelineVertexInputStateCreateInfo vertex_input_info {};

        vk::VertexInputBindingDescription binding_description {};
        binding_description.binding = 0;
        binding_description.stride = sizeof(vertex);
        binding_description.inputRate = vk::VertexInputRate::eVertex;
        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.pVertexBindingDescriptions = &binding_description;

        constexpr eastl::array<vk::VertexInputAttributeDescription, 3> vertex_attributes {
            vk::VertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(vertex, pos)
            },
            vk::VertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(vertex, uv)
            },
            vk::VertexInputAttributeDescription {
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(vertex, color)
            },
        };

        vertex_input_info.vertexAttributeDescriptionCount = vertex_attributes.size();
        vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();

        pipeline_info.pVertexInputState = &vertex_input_info;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        input_assembly_state.primitiveRestartEnable = vk::False;
        pipeline_info.pInputAssemblyState = &input_assembly_state;

        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        rasterization_state.polygonMode = vk::PolygonMode::eFill;
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
        rasterization_state.frontFace = vk::FrontFace::eClockwise;
        rasterization_state.depthClampEnable = vk::False;
        rasterization_state.rasterizerDiscardEnable = vk::False;
        rasterization_state.depthBiasEnable = vk::False;
        rasterization_state.lineWidth = 1.0f;

        pipeline_info.pRasterizationState = &rasterization_state;

        // Enable dynamic states
        // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
        // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
        // For this example we will set the viewport and scissor using dynamic states
        constexpr eastl::array<vk::DynamicState, 2> dynamic_states {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();
        pipeline_info.pDynamicState = &dynamic_state;

        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        multisample_state.rasterizationSamples = vkb::ctx().get_msaa_samples();
        multisample_state.alphaToCoverageEnable = vk::True;
        multisample_state.pSampleMask = nullptr;
        pipeline_info.pMultisampleState = &multisample_state;

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

        vk::PipelineColorBlendAttachmentState blend_attachment_state {};
        blend_attachment_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        blend_attachment_state.blendEnable = vk::True;
        blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        blend_attachment_state.colorBlendOp = vk::BlendOp::eAdd;
        blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        blend_attachment_state.alphaBlendOp = vk::BlendOp::eAdd;

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;;
        pipeline_info.pColorBlendState = &color_blend_state;

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info {};
        const eastl::array<vk::DescriptorSetLayout, 1> set_layouts {
            m_descriptor_set_layout,
        };
        pipeline_layout_create_info.setLayoutCount = static_cast<std::uint32_t>(set_layouts.size());
        pipeline_layout_create_info.pSetLayouts = set_layouts.data();

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eFragment;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(XMFLOAT4) * 2;
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;
        vkcheck(device.createPipelineLayout(&pipeline_layout_create_info, vkb::get_alloc(), &m_pipeline_layout));

        pipeline_info.renderPass = pass;
        pipeline_info.pDepthStencilState = &depth_stencil_state;
        pipeline_info.layout = m_pipeline_layout;

        // Line list depth test enabled pipeline
        depth_stencil_state.depthTestEnable = vk::False;
        depth_stencil_state.depthWriteEnable = vk::False;
        input_assembly_state.topology = vk::PrimitiveTopology::eLineList;
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_info, vkb::get_alloc(), &m_line_depth_pipeline));

        // Line list depth test disabled pipeline
        depth_stencil_state.depthTestEnable = vk::True;
        depth_stencil_state.depthWriteEnable = vk::True;
        input_assembly_state.topology = vk::PrimitiveTopology::eLineList;
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_info, vkb::get_alloc(), &m_line_no_depth_pipeline));

        // Line strip depth test enable pipeline
        depth_stencil_state.depthTestEnable = vk::True;
        depth_stencil_state.depthWriteEnable = vk::True;
        input_assembly_state.topology = vk::PrimitiveTopology::eLineStrip;
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_info, vkb::get_alloc(), &m_line_strip_depth_pipeline));

        // Line strip depth test disable pipeline
        depth_stencil_state.depthTestEnable = vk::False;
        depth_stencil_state.depthWriteEnable = vk::False;
        input_assembly_state.topology = vk::PrimitiveTopology::eLineStrip;
        rasterization_state.cullMode = vk::CullModeFlagBits::eNone;
        vkcheck(device.createGraphicsPipelines(nullptr, 1, &pipeline_info, vkb::get_alloc(), &m_line_strip_no_depth_pipeline));

        device.destroyShaderModule(vs, vkb::get_alloc());
        device.destroyShaderModule(fs, vkb::get_alloc());
    }

    debugdraw::debugdraw() : k_max_vertices{static_cast<std::uint32_t>(k_debug_draw_max_verts())} {
        m_vertices.reserve(k_max_vertices);
        m_draw_commands.reserve(k_max_vertices>>1);
        create_uniform_buffer();
        create_vertex_buffer();
        create_descriptor();
        create_pipeline_states(vkb::ctx().get_device(), vkb::ctx().get_scene_render_pass());
        log_info("Created debug draw context");
    }

    debugdraw::~debugdraw() {
        const vk::Device device = vkb::ctx().get_device();
        device.destroyPipelineLayout(m_pipeline_layout, vkb::get_alloc());
        device.destroyDescriptorSetLayout(m_descriptor_set_layout, vkb::get_alloc());
        device.destroyPipeline(m_line_depth_pipeline, vkb::get_alloc());
        device.destroyPipeline(m_line_no_depth_pipeline, vkb::get_alloc());
        device.destroyPipeline(m_line_strip_depth_pipeline, vkb::get_alloc());
        device.destroyPipeline(m_line_strip_no_depth_pipeline, vkb::get_alloc());
    }
}
