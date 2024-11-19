// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "shader_compiler.hpp"
#include "pipeline_base.hpp"
#include "../core/system_variable.hpp"
#include <ankerl/unordered_dense.h>

namespace soliton::graphics {
    inline const system_variable<bool> sv_parallel_pipeline_creation { "renderer.parallel_pipeline_creation", false };

    class pipeline_cache final : public no_copy, public no_move {
    public:
        explicit pipeline_cache(vk::Device device);
        ~pipeline_cache();

        [[nodiscard]] auto get_pipelines() const -> const ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>>& { return m_pipelines; }

        template <typename T> requires std::is_base_of_v<pipeline_base, T>
        [[nodiscard]] auto get_pipeline(eastl::string&& name) const -> T& {
            if (!m_pipelines.contains(name)) [[unlikely]] {
                log_error("Pipeline not found in registry: '{}'", name);
                for (const auto& [key, value] : m_pipelines) {
                    log_error("Available pipeline: '{}'", key);
                }
                panic("Pipeline not found in registry: '{}'", name);
            }
            auto* const ptr = dynamic_cast<T*>(&*m_pipelines.at(name));
            panic_assert(ptr != nullptr);
            return *ptr;
        }

        template <typename T> requires std::is_base_of_v<pipeline_base, T>
        auto register_pipeline() -> void {
            auto instance = eastl::make_unique<T>();
            auto name = instance->name;
            auto& base = dynamic_cast<pipeline_base&>(*instance);
            base.shader_cache = m_shader_cache;
            base.create(m_cache);
            m_pipelines[name] = std::move(instance);
        }

        auto invalidate_all() -> void;

        [[nodiscard]] auto get_cache() const -> vk::PipelineCache { return m_cache; }
        [[nodiscard]] auto get_shader_cache() const -> std::shared_ptr<shader_cache> { return m_shader_cache; }

    private:
        std::shared_ptr<shader_cache> m_shader_cache = std::make_shared<shader_cache>();
        ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>> m_pipelines {};
        const vk::Device m_device;
        vk::PipelineCache m_cache {};
    };
}