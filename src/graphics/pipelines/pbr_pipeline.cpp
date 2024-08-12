// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"
#include "../vulkancore/context.hpp"
#include "../../core/kernel.hpp"

namespace lu::graphics::pipelines {
    auto pbr_pipeline::render_single_mesh(     // WARNING! RENDER THREAD LOCAL
        vkb::command_buffer& cmd,
        const mesh& mesh,
        const com::mesh_renderer& renderer,
        DirectX::FXMMATRIX view_proj_mtx,
        DirectX::CXMMATRIX model_mtx
    ) const noexcept -> void {
        cmd.push_consts_start();

        push_constants_vs pc_vs {};
        DirectX::XMStoreFloat4x4A(&pc_vs.model_view_proj, DirectX::XMMatrixMultiply(model_mtx, view_proj_mtx));
        DirectX::XMStoreFloat4x4A(&pc_vs.normal_matrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, model_mtx)));
        cmd.push_consts(vk::ShaderStageFlagBits::eVertex, pc_vs);

        push_constants_fs pc_fs {};
        DirectX::XMFLOAT4A camera_pos {};
        DirectX::XMStoreFloat4A(&camera_pos, DirectX::XMMatrixInverse(nullptr, view_proj_mtx).r[3]);
        pc_fs.data = camera_pos;
        pc_fs.data.w = static_cast<float>(kernel::get().get_time());
        cmd.push_consts(vk::ShaderStageFlagBits::eFragment, pc_fs);

        const eastl::span<material* const> mats {renderer.materials.data(), renderer.materials.size()};
        cmd.draw_mesh_with_materials(mesh, mats);
    }

    auto pbr_pipeline::on_bind(vkb::command_buffer& cmd) const -> void {
        graphics_pipeline::on_bind(cmd);
        cmd.bind_graphics_descriptor_set(m_pbr_descriptor_set, 1);
    }

    pbr_pipeline::pbr_pipeline() : graphics_pipeline{"mat_pbr"} {
        vkb::descriptor_factory df {vkb::ctx().descriptor_factory_begin()};

        eastl::array<vk::DescriptorImageInfo, 2> infos {
            vk::DescriptorImageInfo {
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .imageView = m_pbr_filter_processor.brdf_lut().image_view(),
                .sampler = m_pbr_filter_processor.brdf_lut().sampler(),
            },
            vk::DescriptorImageInfo {
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                .imageView = m_pbr_filter_processor.environ_cube().image_view(),
                .sampler = m_pbr_filter_processor.environ_cube().sampler(),
            }
        };

        for (std::size_t i = 0; i < infos.size(); ++i) {
            df.bind_images(i, 1, &infos[i], vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        }

        passert(df.build(m_pbr_descriptor_set, m_pbr_descriptor_set_layout));
    }

    pbr_pipeline::~pbr_pipeline() {
        vkb::vkdvc().destroyDescriptorSetLayout(m_pbr_descriptor_set_layout, vkb::get_alloc());
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
        layouts.emplace_back(material::get_static_resources().descriptor_layout);
        layouts.emplace_back(m_pbr_descriptor_set_layout);

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

    auto pbr_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void {
        passert(type == pipeline_type::graphics);
        cfg.rasterizationSamples = vkb::k_msaa_sample_count;
        cfg.alphaToCoverageEnable = vk::True;
    }
}
