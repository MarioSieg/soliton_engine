// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "pipeline_base.hpp"
#include "vulkancore/context.hpp"

namespace soliton::graphics {
    pipeline_base::~pipeline_base() {
        const vk::Device device = vkb::ctx().get_device();
        if (m_pipeline && m_layout) { // Destroy old pipeline and layout
            device.destroyPipeline(m_pipeline, vkb::get_alloc());
            device.destroyPipelineLayout(m_layout, vkb::get_alloc());
            m_pipeline = nullptr;
            m_layout = nullptr;
        }
    }

    auto pipeline_base::initialize(const vk::PipelineCache cache) -> bool {
        log_info("Creating graphics pipeline '{}' {}st time", name, ++m_num_creations);
        const auto now = eastl::chrono::high_resolution_clock::now();
        if (!pre_configure()) [[unlikely]] {
            return false;
        }
        if (!create(m_layout, m_pipeline, cache)) [[unlikely]] {
            return false;
        }
        if (!post_configure()) [[unlikely]] {
            return false;
        }
        log_info(
            "Created {} pipeline '{}' in {:.03}s",
            type == pipeline_type::graphics ? "graphics" : "compute",
            name,
            eastl::chrono::duration_cast<eastl::chrono::duration<double>>(eastl::chrono::high_resolution_clock::now() - now).count()
        );
        return true;
    }

    pipeline_base::pipeline_base(eastl::string&& name, const pipeline_type type) : name{std::move(name)}, type{type} {}

    auto pipeline_base::pre_configure() -> bool {
        return true;
    }

    auto pipeline_base::post_configure() -> bool {
        return true;
    }
}
