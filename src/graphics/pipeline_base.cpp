// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline_base.hpp"
#include "vulkancore/context.hpp"

namespace lu::graphics {
    pipeline_base::~pipeline_base() {
        const vk::Device device = vkb::ctx().get_device();
        if (m_pipeline && m_layout) { // Destroy old pipeline and layout
            device.destroyPipeline(m_pipeline, vkb::get_alloc());
            device.destroyPipelineLayout(m_layout, vkb::get_alloc());
            m_pipeline = nullptr;
            m_layout = nullptr;
        }
    }

    auto pipeline_base::create(const vk::PipelineCache cache) -> void {
        log_info("Creating graphics pipeline '{}' {}st time", name, ++m_num_creations);
        const auto now = std::chrono::high_resolution_clock::now();
        const auto device = vkb::vkdvc();
        auto prev_pipeline = m_pipeline;
        auto prev_layout = m_layout;
        auto restore_state = [&] {
            m_pipeline = prev_pipeline;
            m_layout = prev_layout;
        };
        pre_configure();
        create(m_layout, m_pipeline, cache);
        post_configure();
        log_info(
            "Created {} pipeline '{}' in {:.03}s",
            type == pipeline_type::graphics ? "graphics" : "compute",
            name,
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - now).count()
        );
        if (prev_pipeline && prev_layout) { // Destroy old pipeline and layout now that the new one is created
            device.destroyPipeline(prev_pipeline, vkb::get_alloc());
            device.destroyPipelineLayout(prev_layout, vkb::get_alloc());
        }
    }

    pipeline_base::pipeline_base(std::string&& name, const pipeline_type type) : name{std::move(name)}, type{type} {
    }

    auto pipeline_base::pre_configure() -> void {
    }

    auto pipeline_base::post_configure() -> void {
    }

    pipeline_registry::pipeline_registry(const vk::Device device) : m_device{device} {
        passert(device);
        vk::PipelineCacheCreateInfo cache_info {};
        vkcheck(device.createPipelineCache(&cache_info, vkb::get_alloc(), &m_cache));
    }

    pipeline_registry::~pipeline_registry() {
        m_pipelines.clear();
        if (m_cache) {
            m_device.destroyPipelineCache(m_cache, vkb::get_alloc());
            m_cache = nullptr;
        }
    }

    static inline constinit std::atomic_bool s_init;

    auto pipeline_registry::init() -> void {
        if (s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance = std::make_unique<pipeline_registry>(vkb::vkdvc());
        s_init.store(true, std::memory_order_relaxed);
    }

    auto pipeline_registry::shutdown() -> void {
        if (!s_init.load(std::memory_order_relaxed)) {
            return;
        }
        s_instance.reset();
        s_init.store(false, std::memory_order_relaxed);
    }

    auto pipeline_registry::invalidate_all() -> void {
        m_pipelines.clear();
    }

    auto pipeline_registry::try_recreate_all() -> void {
        for (const auto& [name, pipeline] : m_pipelines) {
            pipeline->create(m_cache);
        }
    }
}