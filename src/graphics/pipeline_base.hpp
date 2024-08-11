// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "vulkancore/prelude.hpp"
#include "vulkancore/command_buffer.hpp"

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

        const eastl::string name;
        const pipeline_type type;

        auto create(vk::PipelineCache cache) -> void;
        [[nodiscard]] auto get_layout() const -> vk::PipelineLayout { return m_layout; }
        [[nodiscard]] auto get_pipeline() const -> vk::Pipeline { return m_pipeline; }
        [[nodiscard]] auto get_num_creations() const -> std::uint32_t { return m_num_creations; }

    protected:
        explicit pipeline_base(eastl::string&& name, pipeline_type type);

        virtual auto pre_configure() -> void;
        virtual auto post_configure() -> void;

        virtual auto create(vk::PipelineLayout& out_layout, vk::Pipeline& out_pipeline, vk::PipelineCache cache) -> void = 0;

    private:
        vk::PipelineLayout m_layout {};
        vk::Pipeline m_pipeline {};
        std::uint32_t m_num_creations = 0;
    };
}
