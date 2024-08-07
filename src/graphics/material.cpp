// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "material.hpp"

#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"
#include "../scripting/system_variable.hpp"

namespace lu::graphics {
    using scripting::scripting_subsystem;

    static const system_variable<eastl::string> cv_error_texture {"Renderer.fallbackTexture", eastl::monostate{}};
    static const system_variable<eastl::string> cv_flat_normal {"Renderer.flatNormalTexture", eastl::monostate{}};
    static eastl::optional<material::static_resources> s_resources {};

    material::material(
        texture* albedo_map,
        texture* metallic_roughness_map,
        texture* normal_map,
        texture* ambient_occlusion_map
    ) : asset{assetmgr::asset_source::memory} {

        this->albedo_map = albedo_map;
        this->metallic_roughness_map = metallic_roughness_map;
        this->normal_map = normal_map;
        this->ambient_occlusion_map = ambient_occlusion_map;

        flush_property_updates();
    }

    material::~material() = default;

    auto material::flush_property_updates() -> void {
        eastl::fixed_vector<vk::DescriptorImageInfo, 8> image_infos {};

        const auto make_write_tex_info = [](const texture* tex, const texture& fallback) {
            vk::DescriptorImageInfo info {};
            info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            info.imageView = tex ? tex->get_view() : fallback.get_view();
            info.sampler = tex ? tex->get_sampler() : fallback.get_sampler();
            return info;
        };

        image_infos.emplace_back(make_write_tex_info(albedo_map, s_resources->error_texture));
        image_infos.emplace_back(make_write_tex_info(normal_map, s_resources->flat_normal));
        image_infos.emplace_back(make_write_tex_info(metallic_roughness_map, s_resources->error_texture));
        image_infos.emplace_back(make_write_tex_info(ambient_occlusion_map, s_resources->error_texture));

        vkb::descriptor_factory factory {s_resources->descriptor_layout_cache, s_resources->descriptor_allocator};
        for (std::uint32_t i = 0; auto&& info : image_infos)
            factory.bind_images(i++, 1, &info, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        passert(factory.build(m_descriptor_set));
    }

    auto material::init_static_resources() -> void {
        s_resources.emplace();
    }

    auto material::free_static_resources() -> void {
        s_resources.reset();
    }

    auto material::get_static_resources() -> const static_resources& {
        return *s_resources;
    }

    material::static_resources::static_resources() :
        error_texture{cv_error_texture()},
        flat_normal{cv_flat_normal()},
        descriptor_allocator {},
        descriptor_layout_cache {} {

        descriptor_allocator.configured_pool_sizes.clear();
        descriptor_allocator.configured_pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, 1.0f);

        vkb::descriptor_factory factory {vkb::ctx().descriptor_factory_begin()};
        for (std::uint32_t i = 0; i < 4; ++i)
            factory.bind_no_info_stage(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, i);

        vk::DescriptorSet set {};
        passert(factory.build(set, descriptor_layout));
    }

    material::static_resources::~static_resources() {
        vkb::vkdvc().destroyDescriptorSetLayout(descriptor_layout, vkb::get_alloc());
    }
}
