// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <DirectXMath.h>

#include "../pipeline.hpp"

namespace graphics::pipelines {
    class pbr_pipeline final : public pipeline_base {
    public:
        explicit pbr_pipeline();
        ~pbr_pipeline() override;

        struct gpu_vertex_push_constants final {
            DirectX::XMFLOAT4X4A model_view_proj;
            DirectX::XMFLOAT4X4A normal_matrix;
        };
        static_assert(sizeof(gpu_vertex_push_constants) <= 128);

    protected:
        virtual auto pre_configure() -> bool override;
        virtual auto configure_shaders(std::vector<std::pair<std::unique_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> bool;
        virtual auto configure_vertex_info(std::vector<vk::VertexInputBindingDescription>& cfg, std::vector<vk::VertexInputAttributeDescription>& bindings) -> bool;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> bool;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> bool override;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> bool override;

    private:
        vk::DescriptorSetLayout m_descriptor_set_layout {};
    };
}
