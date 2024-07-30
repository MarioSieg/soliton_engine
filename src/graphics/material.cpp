// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "material.hpp"

#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"
#include "../scripting/convar.hpp"

namespace lu::graphics {
    using scripting::scripting_subsystem;

    static convar<eastl::string> cv_error_texture {"Renderer.fallbackTexture", eastl::nullopt, scripting::convar_flags::read_only};
    static convar<eastl::string> cv_flat_normal {"Renderer.flatNormalTexture", eastl::nullopt, scripting::convar_flags::read_only};

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

        passert(s_descriptor_set_layout);

        vk::DescriptorSetAllocateInfo descriptor_set_alloc_info {};
        descriptor_set_alloc_info.descriptorPool = s_descriptor_pool;
        descriptor_set_alloc_info.descriptorSetCount = 1;
        descriptor_set_alloc_info.pSetLayouts = &s_descriptor_set_layout;
        vkcheck(vkb::ctx().get_device().get_logical_device().allocateDescriptorSets(&descriptor_set_alloc_info, &m_descriptor_set));

        flush_property_updates();
    }

    material::~material() = default;

    auto material::flush_property_updates() const -> void {
        eastl::array<vk::DescriptorImageInfo, 4> image_infos {};
        auto make_write_tex_info = [i=0u, this, &image_infos](const texture* tex, const texture* fallback) mutable -> vk::WriteDescriptorSet {
            passert(fallback != nullptr);
            image_infos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            image_infos[i].imageView = tex ? tex->get_view() : fallback->get_view();
            image_infos[i].sampler = tex ? tex->get_sampler() : fallback->get_sampler();
            const vk::WriteDescriptorSet result {
                .dstSet = m_descriptor_set,
                .dstBinding = i,
                .dstArrayElement = 0u,
                .descriptorCount = 1u,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_infos[i],
                .pBufferInfo = nullptr
            };
            ++i;
            return result;
        };
        const eastl::array<vk::WriteDescriptorSet, 4> write_descriptor_sets = {
            make_write_tex_info(albedo_map, &*s_error_texture),
            make_write_tex_info(normal_map, &*s_flat_normal),
            make_write_tex_info(metallic_roughness_map, &*s_error_texture),
            make_write_tex_info(ambient_occlusion_map, &*s_error_texture)
        };
        vkb::vkdvc().updateDescriptorSets(
            static_cast<std::uint32_t>(write_descriptor_sets.size()),
            write_descriptor_sets.data(),
            0u,
            nullptr
        );
    }

    auto material::init_static_resources() -> void {
        s_error_texture.emplace(cv_error_texture());
        s_flat_normal.emplace(cv_flat_normal());

        constexpr unsigned lim = 16384u*4;
        eastl::array<vk::DescriptorPoolSize, 1> pool_sizes = {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = lim
            }
        };

        vk::DescriptorPoolCreateInfo descriptor_pool_create_info {};
        descriptor_pool_create_info.maxSets = lim;
        descriptor_pool_create_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
        vkcheck(vkb::vkdvc().createDescriptorPool(&descriptor_pool_create_info, vkb::get_alloc(), &s_descriptor_pool));

        auto get_texture_binding = [i = 0] () mutable -> vk::DescriptorSetLayoutBinding {
            vk::DescriptorSetLayoutBinding binding {};
            binding.binding = i++;
            binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            binding.descriptorCount = 1;
            binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            return binding;
        };

        const eastl::array<const vk::DescriptorSetLayoutBinding, 4> bindings = {
            get_texture_binding(),
            get_texture_binding(),
            get_texture_binding(),
            get_texture_binding()
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {};
        descriptor_set_layout_create_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor_set_layout_create_info.pBindings = bindings.data();
        vkcheck(vkb::vkdvc().createDescriptorSetLayout(&descriptor_set_layout_create_info, vkb::get_alloc(), &s_descriptor_set_layout));
    }

    auto material::free_static_resources() -> void {
        vkb::vkdvc().destroyDescriptorSetLayout(s_descriptor_set_layout, vkb::get_alloc());
        vkb::vkdvc().destroyDescriptorPool(s_descriptor_pool, vkb::get_alloc());
        s_error_texture.reset();
        s_flat_normal.reset();
    }
}
