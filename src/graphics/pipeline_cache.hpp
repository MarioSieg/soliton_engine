// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "shader.hpp"
#include "pipeline_base.hpp"

namespace lu::graphics {
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