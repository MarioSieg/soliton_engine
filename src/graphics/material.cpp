// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "material.hpp"
#include "../scene/scene_mgr.hpp"
#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"

namespace soliton::graphics {
    static eastl::optional<material::static_resources> s_resources {};

    material::material() : asset{assetmgr::asset_source::memory} {

    }

    material::~material() = default;

    auto material::flush_property_updates(scene& scene) -> void {
        auto& reg = scene.get_asset_registry<graphics::texture>();
        const auto make_write_tex_info = [&reg](const assetmgr::asset_ref texture) {
            auto* texture_ptr = reg[texture];
            panic_assert(texture_ptr != nullptr);
            vk::DescriptorImageInfo info {};
            info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            info.imageView = texture_ptr->image_view();
            info.sampler = texture_ptr->sampler();
            return info;
        };

        eastl::vector<vk::DescriptorImageInfo> image_infos {};
        vkb::descriptor_factory factory {s_resources->descriptor_layout_cache, s_resources->descriptor_allocator};

        for (auto&& [key, prop] : properties) {
            image_infos.emplace_back(make_write_tex_info(prop.value));
        }

        for (std::size_t i = 0; auto&& [_, prop] : properties) {
            factory.bind_images(
                prop.shader_binding,
                1,
                &image_infos[i++],
                vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment
            );
        }

        panic_assert(factory.build(m_descriptor_set));
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

    auto material::print_properties() const -> void {
        log_info("Name | Shader Binding | Value");
        for (auto&& [key, prop] : properties) {
            log_info("'{}' : {} -> {:#x}", key, prop.shader_binding, static_cast<std::underlying_type_t<assetmgr::asset_ref>>(prop.value));
        }
    }

    material::static_resources::static_resources() :
        descriptor_allocator {},
        descriptor_layout_cache {} {

        descriptor_allocator.configured_pool_sizes.clear();
        descriptor_allocator.configured_pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, 1.0f);

        vkb::descriptor_factory factory {vkb::ctx().descriptor_factory_begin()};
        for (std::uint32_t i = 0; i < 6; ++i)
            factory.bind_no_info_stage(vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, i);

        vk::DescriptorSet set {};
        panic_assert(factory.build(set, descriptor_layout));
    }

    material::static_resources::~static_resources() {
        vkb::vkdvc().destroyDescriptorSetLayout(descriptor_layout, vkb::get_alloc());
    }
}
