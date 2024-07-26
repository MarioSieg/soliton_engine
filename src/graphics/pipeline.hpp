// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "shader.hpp"

namespace lu::graphics {
    class mesh;
    class material;

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

        HOTPROC static auto draw_mesh(
            const mesh& mesh,
            const vk::CommandBuffer cmd,
            const std::vector<material*>& mats,
            const vk::PipelineLayout layout
        ) -> void;
        HOTPROC static auto draw_mesh(
            const mesh& mesh,
            const vk::CommandBuffer cmd
        ) -> void;

        virtual auto pre_configure() -> void;
        virtual auto configure_shaders(std::vector<std::shared_ptr<shader>>& cfg) -> void = 0;
        virtual auto configure_pipeline_layout(std::vector<vk::DescriptorSetLayout>& layouts, std::vector<vk::PushConstantRange>& ranges) -> void = 0;
        virtual auto configure_vertex_info(std::vector<vk::VertexInputBindingDescription>& cfg, std::vector<vk::VertexInputAttributeDescription>& bindings) -> void;
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

        [[nodiscard]] auto get_pipelines() const -> const ankerl::unordered_dense::map<std::string, std::unique_ptr<pipeline_base>>& { return m_pipelines; }
        [[nodiscard]] auto get_pipeline(std::string&& name) -> pipeline_base& {
            if (!m_pipelines.contains(name)) [[unlikely]] {
                log_error("Pipeline not found in registry: '{}'", name);
                for (const auto& [key, value] : m_pipelines) {
                    log_error("Available pipeline: '{}'", key);
                }
                panic("Pipeline not found in registry: '{}'", name);
            }
            return *m_pipelines.at(name);
        }

        template<typename T, typename... Args> requires std::is_base_of_v<pipeline_base, T>
        auto register_pipeline(Args&&... args) -> T& {
            auto instance = std::make_unique<T>(std::forward<Args>(args)...);
            passert(!m_pipelines.contains(instance->name));
            auto name = instance->name;
            instance->create(m_cache);
            m_pipelines[name] = std::move(instance);
            return *static_cast<T*>(&*m_pipelines[name]);
        }

        auto invalidate_all() -> void;
        auto try_recreate_all() -> void;

        [[nodiscard]] auto get_cache() const -> vk::PipelineCache { return m_cache; }

        [[nodiscard]] static auto get() -> pipeline_registry& {
            assert(s_instance);
            return *s_instance;
        }

        static auto init() -> void;
        static auto shutdown() -> void;

    private:
        static inline constinit std::unique_ptr<pipeline_registry> s_instance {};
        ankerl::unordered_dense::map<std::string, std::unique_ptr<pipeline_base>> m_pipelines {};
        const vk::Device m_device;
        vk::PipelineCache m_cache {};
    };
}
