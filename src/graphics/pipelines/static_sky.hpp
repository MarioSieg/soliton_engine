// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../graphics_pipeline.hpp"
#include "../texture.hpp"
#include "../mesh.hpp"
#include "../vulkancore/command_buffer.hpp"
#include "../../scene/components.hpp"

namespace soliton::graphics::pipelines {
    class static_sky_pipeline final : public graphics_pipeline {
    public:
        explicit static_sky_pipeline();
        ~static_sky_pipeline() override;

        struct gpu_vertex_push_constants final {
            XMFLOAT4X4A view;
            XMFLOAT4X4A proj;
        };
        static_assert(sizeof(gpu_vertex_push_constants) <= 128);

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void override;

        auto render_sky(vkb::command_buffer& cmd) const -> void;

    protected:
        [[nodiscard]] virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> bool override;
        [[nodiscard]] virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> bool override;
        [[nodiscard]] virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool override;
        [[nodiscard]] virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> bool override;

    private:
        eastl::optional<texture> m_skybox_texture {};
        eastl::optional<mesh> m_skydome {};
        vk::DescriptorPool m_descriptor_pool {};
        vk::DescriptorSet m_descriptor_set {};
        vk::DescriptorSetLayout m_descriptor_set_layout {};
    };
}
