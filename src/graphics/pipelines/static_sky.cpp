// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "static_sky.hpp"
#include "../vulkancore/context.hpp"
#include "../graphics_subsystem.hpp"

namespace soliton::graphics::pipelines {
    static_sky_pipeline::static_sky_pipeline() : graphics_pipeline{"static_sky"} {
        m_skybox_texture.emplace("/RES/textures/hdr/gcanyon_cube.ktx");
        m_skydome.emplace("/RES/meshes/skydome.fbx", false);

        vkb::descriptor_factory factory {vkb::ctx().descriptor_factory_begin()};
        vk::DescriptorImageInfo info {};
        info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        info.imageView = m_skybox_texture->image_view();
        info.sampler = m_skybox_texture->sampler();
        factory.bind_images(0, 1, &info, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        panic_assert(factory.build(m_descriptor_set, m_descriptor_set_layout));
    }

    static_sky_pipeline::~static_sky_pipeline() {
        const vk::Device device = vkb::vkdvc();
        device.destroyDescriptorSetLayout(m_descriptor_set_layout, vkb::get_alloc());
    }

    auto static_sky_pipeline::configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> void {
        auto vs = shader_cache->get_shader(shader_variant{"skybox.vert", shader_stage::vertex});
        auto fs = shader_cache->get_shader(shader_variant{"skybox.frag", shader_stage::fragment});
        cfg.emplace_back(vs);
        cfg.emplace_back(fs);
    }

    auto static_sky_pipeline::configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> void {
        layouts.emplace_back(m_descriptor_set_layout);

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);
        ranges.emplace_back(push_constant_range);
    }

    auto static_sky_pipeline::configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void {
        graphics_pipeline::configure_rasterizer(cfg);
        cfg.cullMode = vk::CullModeFlagBits::eBack;
    }

    auto static_sky_pipeline::on_bind(vkb::command_buffer& cmd) const -> void {
        graphics_pipeline::on_bind(cmd);
        cmd.bind_graphics_descriptor_set(m_descriptor_set, 0, nullptr);
    }

    auto static_sky_pipeline::render_sky(vkb::command_buffer& cmd) const -> void {
        gpu_vertex_push_constants push_constants {};
        XMStoreFloat4x4A(&push_constants.view, XMLoadFloat4x4A(&graphics_subsystem::get_view_mtx()));
        XMStoreFloat4x4A(&push_constants.proj, XMLoadFloat4x4A(&graphics_subsystem::get_proj_mtx()));
        cmd.push_consts_start();
        cmd.push_consts(vk::ShaderStageFlagBits::eVertex, push_constants);
        cmd.draw_mesh(*m_skydome);
    }

    auto static_sky_pipeline::configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void {
        graphics_pipeline::configure_depth_stencil(cfg);
        cfg.depthTestEnable = vk::False;
        cfg.depthWriteEnable = vk::False;
    }
}
