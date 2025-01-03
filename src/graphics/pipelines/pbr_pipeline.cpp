// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"
#include "../vulkancore/context.hpp"
#include "../../core/kernel.hpp"
#include "../graphics_subsystem.hpp"

namespace soliton::graphics::pipelines {
    auto pbr_pipeline::render_single_mesh(     // WARNING! RENDER THREAD LOCAL
        vkb::command_buffer& cmd,
        const mesh& mesh,
        const com::mesh_renderer& renderer,
        FXMMATRIX view_proj_mtx,
        CXMMATRIX model_mtx,
        CXMMATRIX view_mtx
    ) const noexcept -> void {
        cmd.push_consts_start();

        push_constants_vs pc_vs {};
        XMStoreFloat4x4A(&pc_vs.model_matrix, model_mtx);
        XMStoreFloat4x4A(&pc_vs.model_view_proj, XMMatrixMultiply(model_mtx, view_proj_mtx));
        XMStoreFloat4x4A(&pc_vs.normal_matrix, XMMatrixTranspose(XMMatrixInverse(nullptr, model_mtx)));
        cmd.push_consts(vk::ShaderStageFlagBits::eVertex, pc_vs);

        const eastl::span<material* const> mats {renderer.materials.data(), renderer.materials.size()};
        cmd.draw_mesh_with_materials(mesh, mats);
    }

    HOTPROC auto pbr_pipeline::render_mesh_renderer(     // WARNING! RENDER THREAD LOCAL
        vkb::command_buffer& cmd,
        const com::transform& transform,
        const com::mesh_renderer& renderer,
        const BoundingFrustum& frustum,
        FXMMATRIX view_proj_mtx,
        CXMMATRIX view_mtx
    ) const noexcept -> void {
        if (renderer.meshes.empty() || renderer.materials.empty()) // No mesh or material
            return;

        if (renderer.flags & com::render_flags::skip_rendering) // Skip rendering
            return;

        const XMMATRIX model_mtx = transform.compute_matrix();

        for (const mesh* const mesh : renderer.meshes) {
            // Skip mesh if it's null
            if (!mesh) continue;

            // Perform CPU frustum culling
            BoundingOrientedBox obb {};
            obb.CreateFromBoundingBox(obb, mesh->get_aabb());
            obb.Transform(obb, model_mtx);
            if ((renderer.flags & com::render_flags::skip_frustum_culling) == 0) {
                if (frustum.Contains(obb) == ContainmentType::DISJOINT) {
                    continue;
                }
            }
            render_single_mesh(
                cmd,
                *mesh,
                renderer,
                view_proj_mtx,
                model_mtx,
                view_mtx
            );
        }
    }

    auto pbr_pipeline::on_bind(vkb::command_buffer& cmd) const -> void {
        graphics_pipeline::on_bind(cmd);
        const std::uint32_t dynamic_offset = shared_buffers::get()->per_frame_ubo.get_dynamic_aligned_size() * vkb::ctx().get_current_concurrent_frame_idx();
        cmd.bind_graphics_descriptor_set(shared_buffers::get()->get_set(), LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME, &dynamic_offset);
        cmd.bind_graphics_descriptor_set(m_pbr_descriptor_set, LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM);
    }

    pbr_pipeline::pbr_pipeline() : graphics_pipeline{"mat_pbr"} {

    }

    pbr_pipeline::~pbr_pipeline() {
        vkb::vkdvc().destroyDescriptorSetLayout(m_pbr_descriptor_set_layout, vkb::get_alloc());
    }

    auto pbr_pipeline::configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> bool {
        auto vs = shader_cache->get_shader({"pbr_uber_surface.vert", shader_stage::vertex});
        auto fs = shader_cache->get_shader({"pbr_uber_surface.frag", shader_stage::fragment});
        if (!vs || !fs) return false;
        cfg.emplace_back(vs);
        cfg.emplace_back(fs);
        return true;
    }

    auto pbr_pipeline::configure_pipeline_layout(
        eastl::vector<vk::DescriptorSetLayout>& layouts,
        eastl::vector<vk::PushConstantRange>& ranges
    ) -> bool {
        layouts.emplace_back(shared_buffers::get()->get_layout());
        layouts.emplace_back(material::get_static_resources().descriptor_layout);
        layouts.emplace_back(m_pbr_descriptor_set_layout);

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(push_constants_vs);
        ranges.emplace_back(push_constant_range);

        return true;
    }

    auto pbr_pipeline::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> bool {
        graphics_pipeline::configure_enable_color_blending(cfg);
        cfg.blendEnable = vk::False;
        return true;
    }

    auto pbr_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> bool {
        panic_assert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::ctx().get_msaa_samples();
        //cfg.alphaToCoverageEnable = vk::True; // TODO Custom alpha pipeline
        return true;
    }

    auto pbr_pipeline::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool {
        if (!graphics_pipeline::configure_rasterizer(cfg)) return false;
        cfg.cullMode = vk::CullModeFlagBits::eBack;
        return true;
    }

    auto pbr_pipeline::pre_configure() -> bool {
        m_pbr_filter_processor.emplace(shader_cache);
        vkb::descriptor_factory df {vkb::ctx().descriptor_factory_begin()};
        eastl::array<vk::DescriptorImageInfo, 3> infos {
            vk::DescriptorImageInfo {
                .sampler = m_pbr_filter_processor->brdf_lut().sampler(),
                .imageView = m_pbr_filter_processor->brdf_lut().image_view(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            },
            vk::DescriptorImageInfo {
                .sampler = m_pbr_filter_processor->irradiance_cube().sampler(),
                .imageView = m_pbr_filter_processor->irradiance_cube().image_view(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            },
            vk::DescriptorImageInfo {
                .sampler = m_pbr_filter_processor->prefiltered_cube().sampler(),
                .imageView = m_pbr_filter_processor->prefiltered_cube().image_view(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            }
        };

        for (std::size_t i = 0; i < infos.size(); ++i) {
            df.bind_images(i, 1, &infos[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        }

        return df.build(m_pbr_descriptor_set, m_pbr_descriptor_set_layout);
    }
}
