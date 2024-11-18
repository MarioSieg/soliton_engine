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

    auto pipeline_cache::init() -> void {
        [[maybe_unused]] volatile shader_cache& _ensure_init = shader_cache::get(); // ensure shader cache is init
        panic_assert(s_instance == nullptr);
        s_instance = eastl::make_unique<pipeline_cache>(vkb::vkdvc());
    }

    auto pipeline_cache::shutdown() -> void {
        s_instance.reset();
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

    auto pipeline_cache::await_all_pipelines_async() -> void {
        for (auto&& pipe : m_async_load_queue) {
            auto [name, instance] = pipe.get();
            m_pipelines[name] = std::move(instance);
        }
        m_async_load_queue.clear();
    }
}
