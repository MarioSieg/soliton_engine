// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "shader.hpp"
#include "pipeline_base.hpp"

namespace lu::graphics {
    class pipeline_cache final : public no_copy, public no_move {
    public:
        explicit pipeline_cache(vk::Device device);
        ~pipeline_cache();

        [[nodiscard]] auto get_pipelines() const -> const ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>>& { return m_pipelines; }
        [[nodiscard]] auto get_pipeline(eastl::string&& name) -> pipeline_base& {
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
            auto instance = eastl::make_unique<T>(std::forward<Args>(args)...);
            passert(!m_pipelines.contains(instance->name));
            auto name = instance->name;
            static_cast<pipeline_base&>(*instance).create(m_cache);
            m_pipelines[name] = std::move(instance);
            return *static_cast<T*>(&*m_pipelines[name]);
        }

        auto invalidate_all() -> void;
        auto recreate_all() -> void;

        [[nodiscard]] auto get_cache() const -> vk::PipelineCache { return m_cache; }

        [[nodiscard]] static auto get() -> pipeline_cache& {
            assert(s_instance);
            return *s_instance;
        }

        static auto init() -> void;
        static auto shutdown() -> void;

    private:
        static inline eastl::unique_ptr<pipeline_cache> s_instance {};
        ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>> m_pipelines {};
        const vk::Device m_device;
        vk::PipelineCache m_cache {};
    };
}