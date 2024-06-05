// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "sky.hpp"
#include "../shader_registry.hpp"
#include "../vulkancore/context.hpp"
#include "../graphics_subsystem.hpp"

namespace graphics::pipelines {
    sky_pipeline::sky_pipeline() : pipeline_base{"sky", pipeline_type::graphics} {
        m_skybox_texture.emplace("assets/textures/hdr/gcanyon_cube.ktx");
        m_skydome.emplace("assets/meshes/bunny.obj");

        const vk::Device device = vkb::vkdvc();

        constexpr std::array<vk::DescriptorSetLayoutBinding, 1> bindings {
            vk::DescriptorSetLayoutBinding {
                .binding = 0u,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1u,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr
            }
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_layout {};
        descriptor_layout.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor_layout.pBindings = bindings.data();
        vkcheck(device.createDescriptorSetLayout(&descriptor_layout, vkb::get_alloc(), &m_descriptor_set_layout));

        vk::DescriptorPoolSize pool_size {};
        pool_size.type = vk::DescriptorType::eCombinedImageSampler;
        pool_size.descriptorCount = 2u;


        const vk::DescriptorPoolCreateInfo pool_info {
            .maxSets = 1u,
            .poolSizeCount = 1u,
            .pPoolSizes = &pool_size
        };
        vkcheck(device.createDescriptorPool(&pool_info, vkb::get_alloc(), &m_descriptor_pool));

        vk::DescriptorSetAllocateInfo alloc_info {};
        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1u;
        alloc_info.pSetLayouts = &m_descriptor_set_layout;
        vkcheck(device.allocateDescriptorSets(&alloc_info, &m_descriptor_set));

        vk::DescriptorImageInfo image_info {};
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = m_skybox_texture->get_view();
        image_info.sampler = m_skybox_texture->get_sampler();

        vk::WriteDescriptorSet write {};
        write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        write.descriptorCount = 1u;
        write.dstBinding = 0u;
        write.dstArrayElement = 0u;
        write.dstSet = m_descriptor_set;
        write.pImageInfo = &image_info;
        device.updateDescriptorSets(1u, &write, 0u, nullptr);
    }

    sky_pipeline::~sky_pipeline() {
        const vk::Device device = vkb::vkdvc();
        device.destroyDescriptorSetLayout(m_descriptor_set_layout, vkb::get_alloc());
    }

    auto sky_pipeline::configure_shaders(std::vector<std::pair<std::shared_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> void {
        auto vs = shader_registry::get().get_shader("skybox.vert");
        auto fs = shader_registry::get().get_shader("skybox.frag");
        cfg.emplace_back(vs, vk::ShaderStageFlagBits::eVertex);
        cfg.emplace_back(fs, vk::ShaderStageFlagBits::eFragment);
    }

    auto sky_pipeline::configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void {
        layouts.emplace_back(m_descriptor_set_layout);

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);
        ranges.emplace_back(push_constant_range);
    }

    auto sky_pipeline::render(const vk::CommandBuffer cmd) const -> void {
        const vk::Device device = vkb::vkdvc();
        const vk::PipelineLayout layout = get_layout();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0u, 1u, &m_descriptor_set, 0u, nullptr);
        gpu_vertex_push_constants push_constants {};
        const DirectX::XMMATRIX model = DirectX::XMMatrixScalingFromVector(DirectX::XMVectorReplicate(10.0f));
        const auto& vp = graphics_subsystem::get_view_proj_mtx();
        DirectX::XMStoreFloat4x4A(&push_constants.model_view_proj, DirectX::XMMatrixMultiply(model, DirectX::XMLoadFloat4x4A(&vp)));
        cmd.pushConstants(
            layout,
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(push_constants),
            &push_constants
        );
        draw_mesh(*m_skydome, cmd);
    }
}
