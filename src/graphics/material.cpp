// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "material.hpp"

#include "vulkancore/context.hpp"
#include "graphics_subsystem.hpp"

namespace graphics {
    material::material(
        texture* albedo_map,
        texture* metallic_roughness_map,
        texture* normal_map,
        texture* ambient_occlusion_map
    ) : asset{asset_category::material, asset_source::memory} {
        this->albedo_map = albedo_map;
        this->metallic_roughness_map = metallic_roughness_map;
        this->normal_map = normal_map;
        this->ambient_occlusion_map = ambient_occlusion_map;

        passert(s_descriptor_set_layout);

        vk::DescriptorSetAllocateInfo descriptor_set_alloc_info {};
        descriptor_set_alloc_info.descriptorPool = s_descriptor_pool;
        descriptor_set_alloc_info.descriptorSetCount = 1;
        descriptor_set_alloc_info.pSetLayouts = &s_descriptor_set_layout;
        vkcheck(vkb::context::s_instance->get_device().get_logical_device().allocateDescriptorSets(&descriptor_set_alloc_info, &m_descriptor_set));

        flush_property_updates();
    }

    material::~material() = default;

    auto material::flush_property_updates() const -> void {
        std::array<vk::DescriptorImageInfo, 4> image_infos {};
        auto make_write_tex_info = [i = 0u, this, &image_infos](const texture* tex) mutable -> vk::WriteDescriptorSet {
            image_infos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            image_infos[i].imageView = tex ? tex->get_view() : s_default_texture->get_view();
            image_infos[i].sampler = tex ? tex->get_sampler() : s_default_texture->get_sampler();
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
        const std::array<vk::WriteDescriptorSet, 4> write_descriptor_sets = {
            make_write_tex_info(albedo_map),
            make_write_tex_info(normal_map),
            make_write_tex_info(metallic_roughness_map),
            make_write_tex_info(ambient_occlusion_map)
        };
        vkb_vk_device().updateDescriptorSets(
            static_cast<std::uint32_t>(write_descriptor_sets.size()),
            write_descriptor_sets.data(),
            0u,
            nullptr
        );
    }

    auto material::init_static_resources() -> void {
        s_default_texture.emplace("assets/textures/system/error.png");

        constexpr unsigned lim = 8192u;
        std::array<vk::DescriptorPoolSize, 1> pool_sizes = {
            vk::DescriptorPoolSize {
                .type = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = lim
            }
        };

        vk::DescriptorPoolCreateInfo descriptor_pool_create_info {};
        descriptor_pool_create_info.maxSets = lim;
        descriptor_pool_create_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
        vkcheck(vkb_vk_device().createDescriptorPool(&descriptor_pool_create_info, &vkb::s_allocator, &s_descriptor_pool));

        auto get_texture_binding = [i = 0] () mutable -> vk::DescriptorSetLayoutBinding {
            vk::DescriptorSetLayoutBinding binding {};
            binding.binding = i++;
            binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            binding.descriptorCount = 1;
            binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            return binding;
        };

        const std::array<const vk::DescriptorSetLayoutBinding, 4> bindings = {
            get_texture_binding(),
            get_texture_binding(),
            get_texture_binding(),
            get_texture_binding()
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {};
        descriptor_set_layout_create_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor_set_layout_create_info.pBindings = bindings.data();
        vkcheck(vkb_vk_device().createDescriptorSetLayout(&descriptor_set_layout_create_info, &vkb::s_allocator, &s_descriptor_set_layout));
    }

    auto material::free_static_resources() -> void {
        vkb_vk_device().destroyDescriptorSetLayout(s_descriptor_set_layout, &vkb::s_allocator);
        vkb_vk_device().destroyDescriptorPool(s_descriptor_pool, &vkb::s_allocator);
        s_default_texture.reset();
    }
}
