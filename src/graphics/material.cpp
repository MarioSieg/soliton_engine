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

        passert(m_descriptor_set_layout);

        vk::DescriptorSetAllocateInfo descriptor_set_alloc_info {};
        descriptor_set_alloc_info.descriptorPool = m_descriptor_pool;
        descriptor_set_alloc_info.descriptorSetCount = 1;
        descriptor_set_alloc_info.pSetLayouts = &m_descriptor_set_layout;
        vkcheck(vkb::context::s_instance->get_device().get_logical_device().allocateDescriptorSets(&descriptor_set_alloc_info, &m_descriptor_set));

        create_sampler();
        flush_property_updates();
    }

    material::~material() {
        vkb_vk_device().destroySampler(m_sampler, &vkb::s_allocator);
    }

    auto material::flush_property_updates() const -> void {
        std::array<vk::DescriptorImageInfo, 4> image_infos {};
        auto make_write_tex_info = [i = 0u, this, &image_infos](const texture* tex) mutable -> vk::WriteDescriptorSet {
            image_infos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            image_infos[i].imageView = tex ? tex->get_view() : m_default_texture->get_view();
            image_infos[i].sampler = m_sampler;
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
            make_write_tex_info(metallic_roughness_map),
            make_write_tex_info(normal_map),
            make_write_tex_info(ambient_occlusion_map)
        };
        vkb_vk_device().updateDescriptorSets(
            static_cast<std::uint32_t>(write_descriptor_sets.size()),
            write_descriptor_sets.data(),
            0u,
            nullptr
        );
    }

    auto material::create_sampler() -> void {
        const bool supports_anisotropy = vkb_device().get_physical_device_features().samplerAnisotropy;

        vk::SamplerCreateInfo sampler_info {};
        sampler_info.magFilter = vk::Filter::eLinear;
        sampler_info.minFilter = vk::Filter::eLinear;
        sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.compareOp = vk::CompareOp::eNever;
        sampler_info.minLod = 0.0f;
        //sampler_info.maxLod = static_cast<float>(m_texture->get_mip_levels());
        sampler_info.maxAnisotropy = supports_anisotropy ? vkb_device().get_physical_device_props().limits.maxSamplerAnisotropy : 1.0f;
        sampler_info.anisotropyEnable = supports_anisotropy ? vk::True : vk::False;
        sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        vkcheck(vkb_vk_device().createSampler(&sampler_info, &vkb::s_allocator, &m_sampler));
    }

    auto material::create_global_descriptors() -> void {
        if (m_descriptor_pool) { // not thread safe
            return;
        }

        m_default_texture = std::make_unique<texture>("proto/red/texture_01.png");

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
        vkcheck(vkb::context::s_instance->get_device().get_logical_device().createDescriptorPool(&descriptor_pool_create_info, &vkb::s_allocator, &m_descriptor_pool));

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
        vkcheck(vkb::context::s_instance->get_device().get_logical_device().createDescriptorSetLayout(&descriptor_set_layout_create_info, &vkb::s_allocator, &m_descriptor_set_layout));
    }
}
