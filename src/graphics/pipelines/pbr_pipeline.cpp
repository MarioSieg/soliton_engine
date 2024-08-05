// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"
#include "../vulkancore/context.hpp"
#include "../../core/kernel.hpp"

namespace lu::graphics::pipelines {
    // WARNING! RENDER THREAD LOCAL
    HOTPROC auto pbr_pipeline::render_mesh(
        vkb::command_buffer& cmd_buf,
        const com::transform& transform,
        const com::mesh_renderer& renderer,
        const DirectX::BoundingFrustum& frustum,
        DirectX::FXMMATRIX vp
    ) const -> void {
        if (renderer.meshes.empty() || renderer.materials.empty()) [[unlikely]] {
            return;
        }
        if (renderer.flags & com::render_flags::skip_rendering) [[unlikely]] {
            return;
        }
        const DirectX::XMMATRIX model = transform.compute_matrix();
        for (const mesh* mesh : renderer.meshes) {
            if (!mesh) [[unlikely]] {
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

            cmd_buf.push_consts_start();

            push_constants_vs pc_vs {};
            DirectX::XMStoreFloat4x4A(&pc_vs.model_view_proj, DirectX::XMMatrixMultiply(model, vp));
            DirectX::XMStoreFloat4x4A(&pc_vs.normal_matrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, model)));
            cmd_buf.push_consts(vk::ShaderStageFlagBits::eVertex, pc_vs);

            push_constants_fs pc_fs {};
            pc_fs.time = static_cast<float>(kernel::get().get_time());
            cmd_buf.push_consts(vk::ShaderStageFlagBits::eFragment, pc_fs);

            const eastl::span<material* const> mats {renderer.materials.data(), renderer.materials.size()};
            cmd_buf.draw_mesh_with_materials(*mesh, mats);
        }
    }

    pbr_pipeline::pbr_pipeline() : graphics_pipeline{"mat_pbr"} {
        generate_brdf_lut();
    }

    pbr_pipeline::~pbr_pipeline() {
        const vk::Device device = vkb::ctx().get_device();
        device.destroyImageView(m_brdf_lut.m_image_view, vkb::get_alloc());
        device.destroyImage(m_brdf_lut.image, vkb::get_alloc());
        device.freeMemory(m_brdf_lut.memory, vkb::get_alloc());
    }

    auto pbr_pipeline::configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void {
        shader_variant vs_variant {"/engine_assets/shaders/src/pbr_uber_surface.vert", shader_stage::vertex};
        shader_variant fs_variant {"/engine_assets/shaders/src/pbr_uber_surface.frag", shader_stage::fragment};
        auto vs = shader_cache::get().get_shader(std::move(vs_variant));
        auto fs = shader_cache::get().get_shader(std::move(fs_variant));
        cfg.emplace_back(vs);
        cfg.emplace_back(fs);
    }

    auto pbr_pipeline::configure_pipeline_layout(
        eastl::vector<vk::DescriptorSetLayout>& layouts,
        eastl::vector<vk::PushConstantRange>& ranges
    ) -> void {
        layouts.emplace_back(material::get_descriptor_set_layout());

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(push_constants_vs);
        ranges.emplace_back(push_constant_range);

        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eFragment;
        push_constant_range.offset = sizeof(push_constants_vs);
        push_constant_range.size = sizeof(push_constants_fs);
        ranges.emplace_back(push_constant_range);
    }

    auto pbr_pipeline::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
        graphics_pipeline::configure_enable_color_blending(cfg);
    }

    auto pbr_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo &cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::k_msaa_sample_count;
        cfg.alphaToCoverageEnable = vk::True;
    }
}
