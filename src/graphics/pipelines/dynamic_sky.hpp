// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../graphics_pipeline.hpp"
#include "../texture.hpp"
#include "../mesh.hpp"
#include "../vulkancore/command_buffer.hpp"
#include "../../scene/components.hpp"

namespace soliton::graphics::pipelines {
    class dynamic_sky_pipeline final : public graphics_pipeline {
    public:
        static constexpr std::size_t grid_size = 32;

        explicit dynamic_sky_pipeline();
        ~dynamic_sky_pipeline() override = default;

        virtual auto on_bind(vkb::command_buffer& cmd) const -> void override;

        auto render_sky(vkb::command_buffer& cmd) const -> void;
        static auto get_ubo_data(glsl::sky_data& data) -> void;

    protected:
        [[nodiscard]] virtual auto configure_shaders(eastl::vector<eastl::shared_ptr<shader>>& cfg) -> bool override;
        [[nodiscard]] virtual auto configure_pipeline_layout(eastl::vector<vk::DescriptorSetLayout>& layouts, eastl::vector<vk::PushConstantRange>& ranges) -> bool override;
        [[nodiscard]] virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> bool override;
        [[nodiscard]] virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> bool override;

    private:
        eastl::optional<mesh> m_grid {};
    };
}
