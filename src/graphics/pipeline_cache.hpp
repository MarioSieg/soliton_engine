// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"

#include <ankerl/unordered_dense.h>

#include "shader.hpp"
#include "pipeline_base.hpp"
#include "../scripting/system_variable.hpp"

namespace lu::graphics {
    inline const system_variable<bool> sv_parallel_pipeline_creation { "renderer.parallel_pipeline_creation", true };

    class pipeline_cache final : public no_copy, public no_move {
    public:
        explicit pipeline_cache(vk::Device device);
        ~pipeline_cache();

        [[nodiscard]] auto get_pipelines() const -> const ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>>& { return m_pipelines; }

        template <typename T> requires std::is_base_of_v<pipeline_base, T>
        [[nodiscard]] auto get_pipeline(eastl::string&& name) -> T& {
            if (!m_pipelines.contains(name)) [[unlikely]] {
                log_error("Pipeline not found in registry: '{}'", name);
                for (const auto& [key, value] : m_pipelines) {
                    log_error("Available pipeline: '{}'", key);
                }
                panic("Pipeline not found in registry: '{}'", name);
            }
            if constexpr (DEBUG) {
                auto* const ptr = dynamic_cast<T*>(&*m_pipelines.at(name));
                passert(ptr != nullptr);
                return *ptr;
            } else {
                return static_cast<T&>(*m_pipelines.at(name));
            }
        }

        template <typename T> requires std::is_base_of_v<pipeline_base, T>
        auto register_pipeline_async() -> void {
            m_async_load_queue.emplace_back(std::async(sv_parallel_pipeline_creation() ? std::launch::async : std::launch::deferred, [this] () -> eastl::pair<eastl::string, eastl::unique_ptr<pipeline_base>> {
                auto instance = eastl::make_unique<T>();
                auto name = instance->name;
                static_cast<pipeline_base&>(*instance).create(m_cache);
                return eastl::make_pair(eastl::move(name), eastl::move(instance));
            }));
        }

        auto await_all_pipelines_async() -> void;
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
        eastl::vector<std::future<eastl::pair<eastl::string, eastl::unique_ptr<pipeline_base>>>> m_async_load_queue {};
        ankerl::unordered_dense::map<eastl::string, eastl::unique_ptr<pipeline_base>> m_pipelines {};
        const vk::Device m_device;
        vk::PipelineCache m_cache {};
    };
}