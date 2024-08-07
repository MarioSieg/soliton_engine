// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline_cache.hpp"
#include "vulkancore/context.hpp"


namespace lu::graphics {
    pipeline_cache::pipeline_cache(const vk::Device device) : m_device{device} {
        passert(device);
        vk::PipelineCacheCreateInfo cache_info {};
        vkcheck(device.createPipelineCache(&cache_info, vkb::get_alloc(), &m_cache));
    }

    pipeline_cache::~pipeline_cache() {
        m_pipelines.clear();
        if (m_cache) {
            m_device.destroyPipelineCache(m_cache, vkb::get_alloc());
            m_cache = nullptr;
        }
    }

    static inline constinit std::atomic_bool s_init;

    auto pipeline_cache::init() -> void {
        if (s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance = eastl::make_unique<pipeline_cache>(vkb::vkdvc());
        s_init.store(true, std::memory_order_relaxed);
    }

    auto pipeline_cache::shutdown() -> void {
        if (!s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance.reset();
        s_init.store(false, std::memory_order_relaxed);
    }

    auto pipeline_cache::invalidate_all() -> void {
        m_pipelines.clear();
    }

    auto pipeline_cache::recreate_all() -> void {
        shader_cache::get().invalidate_all();
        for (const auto& [name, pipeline] : m_pipelines) {
            pipeline->create(m_cache);
        }
    }
}
