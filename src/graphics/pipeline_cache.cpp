// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline_cache.hpp"
#include "vulkancore/context.hpp"


namespace soliton::graphics {
    pipeline_cache::pipeline_cache(const vk::Device device) : m_device{device} {
        panic_assert(device);
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

    auto pipeline_cache::invalidate_all() -> void {
        m_pipelines.clear();
        m_shader_cache->invalidate_all();
    }
}
