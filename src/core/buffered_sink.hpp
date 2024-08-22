// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <spdlog/sinks/base_sink.h>

#include <EASTL/vector.h>
#include <EASTL/span.h>
#include <EASTL/string.h>

namespace lu {
    class buffered_sink final : public spdlog::sinks::base_sink<std::mutex> {
    public:
        explicit buffered_sink(const std::size_t cap) { m_history.reserve(cap); }
        inline auto get() const noexcept -> eastl::span<const eastl::pair<spdlog::level::level_enum, eastl::string>> { return m_history; }
        auto clear() noexcept -> void { m_history.clear(); }

    private:
        eastl::vector<eastl::pair<spdlog::level::level_enum, eastl::string>> m_history{};
        auto sink_it_(const spdlog::details::log_msg& msg) -> void override;
        auto flush_() -> void override {}
    };
}
