// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "shader.hpp"

namespace lu::graphics {
    class mesh;
    class material;

    enum class pipeline_type : std::uint8_t {
        graphics,
        compute
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
        virtual auto post_configure() -> void;

        virtual auto create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> void = 0;

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

        template <typename T, typename... Args> requires std::is_base_of_v<pipeline_base, T>
        auto register_pipeline(Args&&... args) -> T& {
            auto instance = std::make_unique<T>(std::forward<Args>(args)...);
            passert(!m_pipelines.contains(instance->name));
            auto name = instance->name;
            static_cast<pipeline_base&>(*instance).create(m_cache);
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