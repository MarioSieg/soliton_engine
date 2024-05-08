// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"
#include "../vulkancore/context.hpp"
#include "../shader_registry.hpp"

namespace graphics::pipelines {
    // WARNING! RENDER THREAD LOCAL
    HOTPROC auto pbr_pipeline::render_mesh(
        const vk::CommandBuffer cmd_buf,
        const vk::PipelineLayout layout,
        const com::transform& transform,
        const com::mesh_renderer& renderer,
        const DirectX::BoundingFrustum& frustum,
        DirectX::FXMMATRIX vp
    ) const -> void {
        if (renderer.meshes.empty() || renderer.materials.empty()) [[unlikely]] {
            log_warn("Mesh renderer has no meshes or materials");
            return;
        }
        if (renderer.flags & com::render_flags::skip_rendering) [[unlikely]] {
            return;
        }
        const DirectX::XMMATRIX model = transform.compute_matrix();
        for (const mesh* mesh : renderer.meshes) {
            if (!mesh) [[unlikely]] {
                log_warn("Mesh renderer has a null mesh");
                continue;
            }

            // Frustum Culling
            DirectX::BoundingOrientedBox obb {};
            obb.CreateFromBoundingBox(obb, mesh->get_aabb());
            obb.Transform(obb, model);
            if ((renderer.flags & com::render_flags::skip_frustum_culling) == 0) [[likely]] {
                if (frustum.Contains(obb) == DirectX::ContainmentType::DISJOINT) { // Object is culled
                    return;
                }
            }

            // Uniforms
            pipelines::pbr_pipeline::gpu_vertex_push_constants push_constants {};
            DirectX::XMStoreFloat4x4A(&push_constants.model_view_proj, DirectX::XMMatrixMultiply(model, vp));
            DirectX::XMStoreFloat4x4A(&push_constants.normal_matrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, model)));
            cmd_buf.pushConstants(
                layout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(push_constants),
                &push_constants
            );

            draw_mesh(*mesh, cmd_buf, renderer.materials, layout);
        }
    }

    pbr_pipeline::pbr_pipeline() : pipeline_base{"pbr", pipeline_type::graphics} {
        generate_brdf_lut();
    }

    pbr_pipeline::~pbr_pipeline() {
        const vk::Device device = vkb::ctx().get_device();
        device.destroyImageView(m_brdf_lut.m_image_view, &vkb::s_allocator);
        device.destroyImage(m_brdf_lut.image, &vkb::s_allocator);
        device.freeMemory(m_brdf_lut.memory, &vkb::s_allocator);
    }

    auto pbr_pipeline::configure_shaders(std::vector<std::pair<std::shared_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> void {
        auto vs = shader_registry::get().get_shader("pbr_uber_surface.vert");
        auto fs = shader_registry::get().get_shader("pbr_uber_surface.frag");
        cfg.emplace_back(vs, vk::ShaderStageFlagBits::eVertex);
        cfg.emplace_back(fs, vk::ShaderStageFlagBits::eFragment);
    }

    auto pbr_pipeline::configure_pipeline_layout(
        std::vector<vk::DescriptorSetLayout>& layouts,
        std::vector<vk::PushConstantRange>& ranges
    ) -> void {
        layouts.emplace_back(material::get_descriptor_set_layout());

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);
        ranges.emplace_back(push_constant_range);
    }
}
