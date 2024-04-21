// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <chrono>
#include <string>
#include "GLFW/glfw3.h"

extern auto profiler_start_task(std::string&& name, std::uint32_t color = ~0u) -> std::size_t;
extern auto profiler_end_task(std::size_t id) -> void;
extern auto profiler_new_frame() -> void;
[[nodiscard]] extern auto profiler_get_frame_time() noexcept -> double;
extern auto profiler_submit_ex(std::string&& name, double start, double end, std::uint32_t color = ~0u) -> void;

class scoped_profiler_sample final {
public:
    explicit scoped_profiler_sample(std::string&& name, std::uint32_t color = ~0u) noexcept : m_id{profiler_start_task(std::move(name), color)} {
    }

    ~scoped_profiler_sample() {
        profiler_end_task(m_id);
    }

private:
    const std::size_t m_id;
};
