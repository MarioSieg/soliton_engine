// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pbr_pipeline.hpp"
#include "../mesh.hpp"
#include "../material.hpp"
#include "../vulkancore/context.hpp"

namespace graphics::pipelines {
    pbr_pipeline::pbr_pipeline() : pipeline_base{"pbr", pipeline_type::graphics} {

    }

    pbr_pipeline::~pbr_pipeline() {
        const vk::Device device = vkb_context().get_device();
        device.destroyDescriptorSetLayout(m_descriptor_set_layout, &vkb::s_allocator);

    }

    void pbr_pipeline::pre_configure() {
        const vk::Device device = vkb_context().get_device();

        // Binding 0: Uniform buffer (Vertex shader)
        vk::DescriptorSetLayoutBinding layout_binding {};
        layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        layout_binding.descriptorCount = 1;
        layout_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
        layout_binding.pImmutableSamplers = nullptr;

        constexpr std::array<vk::DescriptorSetLayoutBinding, 1> bindings {
            vk::DescriptorSetLayoutBinding {
                .binding = 0u,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1u,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
                .pImmutableSamplers = nullptr
            }
        };

        vk::DescriptorSetLayoutCreateInfo descriptor_layout {};
        descriptor_layout.bindingCount = static_cast<std::uint32_t>(bindings.size());
        descriptor_layout.pBindings = bindings.data();
        vkcheck(device.createDescriptorSetLayout(&descriptor_layout, &vkb::s_allocator, &m_descriptor_set_layout));
    }

    auto pbr_pipeline::configure_shaders(std::vector<std::pair<std::unique_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> void {
        cfg.emplace_back(std::make_unique<vkb::shader>("triangle.vert"), vk::ShaderStageFlagBits::eVertex);
        cfg.emplace_back(std::make_unique<vkb::shader>("triangle.frag"), vk::ShaderStageFlagBits::eFragment);
    }

    auto pbr_pipeline::configure_vertex_info(
        std::vector<vk::VertexInputBindingDescription>& cfg,
        std::vector<vk::VertexInputAttributeDescription>& bindings
    ) -> void {
        cfg.emplace_back(vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(mesh::vertex),
            .inputRate = vk::VertexInputRate::eVertex
        });

        auto push_attribute = [&](const vk::Format format, const std::uint32_t offset) mutable  {
            vk::VertexInputAttributeDescription& desc = bindings.emplace_back();
            desc.location = bindings.size() - 1;
            desc.binding = 0;
            desc.format = format;
            desc.offset = offset;
        };
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, position));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, normal));
        push_attribute(vk::Format::eR32G32Sfloat, offsetof(mesh::vertex, uv));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, tangent));
        push_attribute(vk::Format::eR32G32B32Sfloat, offsetof(mesh::vertex, bitangent));
    }

    auto pbr_pipeline::configure_pipeline_layout(
        std::vector<vk::DescriptorSetLayout>& layouts,
        std::vector<vk::PushConstantRange>& ranges
    ) -> void {
        layouts.emplace_back(m_descriptor_set_layout);
        layouts.emplace_back(material::get_descriptor_set_layout());

        vk::PushConstantRange push_constant_range {};
        push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(gpu_vertex_push_constants);
        ranges.emplace_back(push_constant_range);
    }

    auto pbr_pipeline::configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void {
        pipeline_base::configure_color_blending(cfg);
        cfg.blendEnable = vk::True;
    }

    void pbr_pipeline::configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) {
        pipeline_base::configure_multisampling(cfg);
        cfg.alphaToCoverageEnable = vk::True;
    }
}