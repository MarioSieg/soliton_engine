// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "vulkancore/shader.hpp"

namespace graphics {
    enum class pipeline_type : std::uint8_t {
        graphics
    };

    class pipeline_base : public no_copy, public no_move {
    public:
        virtual ~pipeline_base();

        const std::string name;
        const pipeline_type type;

        auto create(vk::PipelineCache cache) -> void;
        [[nodiscard]] auto get_layout() const -> vk::PipelineLayout { return m_layout; }
        [[nodiscard]] auto get_pipeline() const -> vk::Pipeline { return m_pipeline; }
        [[nodiscard]] auto get_num_creations() const -> std::uint32_t { return m_num_creations; }

    protected:
        explicit pipeline_base(std::string&& name, pipeline_type type);

        virtual auto pre_configure() -> void;
        virtual auto configure_shaders(std::vector<std::pair<std::unique_ptr<vkb::shader>, vk::ShaderStageFlagBits>>& cfg) -> void = 0;
        virtual auto configure_vertex_info(std::vector<vk::VertexInputBindingDescription>& cfg, std::vector<vk::VertexInputAttributeDescription>& bindings) -> void = 0;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void = 0;
        virtual auto configure_viewport_state(vk::PipelineViewportStateCreateInfo& cfg) -> void;
        virtual auto configure_input_assembly(vk::PipelineInputAssemblyStateCreateInfo& cfg) -> void;
        virtual auto configure_rasterizer(vk::PipelineRasterizationStateCreateInfo& cfg) -> void;
        virtual auto configure_dynamic_states(std::vector<vk::DynamicState>& states) -> void;
        virtual auto configure_depth_stencil(vk::PipelineDepthStencilStateCreateInfo& cfg) -> void;
        virtual auto configure_multisampling(vk::PipelineMultisampleStateCreateInfo& cfg) -> void;
        virtual auto configure_color_blending(vk::PipelineColorBlendAttachmentState& cfg) -> void;
        virtual auto configure_render_pass(vk::RenderPass& pass) -> void;
        virtual auto post_configure() -> void;

    private:
        vk::PipelineLayout m_layout {};
        vk::Pipeline m_pipeline {};
        std::uint32_t m_num_creations = 0;
    };

    class pipeline_registry final : public no_copy, public no_move {
    public:
        explicit pipeline_registry(vk::Device device);
        ~pipeline_registry();

        [[nodiscard]] auto get_pipelines() const -> const ankerl::unordered_dense::map<std::string_view, std::unique_ptr<pipeline_base>>& { return m_pipelines; }
        [[nodiscard]] auto get_pipeline(const std::string_view name) -> pipeline_base& {
            passert(m_pipelines.contains(name));
            return *m_pipelines[name];
        }

        template<typename T, typename... Args> requires std::is_base_of_v<pipeline_base, T>
        auto register_pipeline(Args&&... args) -> T& {
            auto instance = std::make_unique<T>(std::forward<Args>(args)...);
            passert(!m_pipelines.contains(instance->name));
            instance->create(m_cache);
            m_names.emplace_back(std::string{instance->name});
            m_pipelines[m_names.back()] = std::move(instance);
            return *static_cast<T*>(&*m_pipelines[m_names.back()]);
        }

        [[nodiscard]] static auto get() -> pipeline_registry& {
            assert(s_instance.has_value());
            return *s_instance;
        }

    private:
        friend class graphics_subsystem;
        static inline constinit std::unique_ptr<pipeline_registry> s_instance {};
        std::vector<std::string> m_names {};
        ankerl::unordered_dense::map<std::string_view, std::unique_ptr<pipeline_base>> m_pipelines {};
        const vk::Device m_device;
        vk::PipelineCache m_cache {};
    };
}
