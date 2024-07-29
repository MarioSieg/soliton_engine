// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <spdlog/sinks/base_sink.h>

namespace lu {
    class buffered_sink final : public spdlog::sinks::base_sink<std::mutex> {
    public:
        explicit buffered_sink(const std::size_t cap) { m_Backtrace.reserve(cap); }

        auto get() const noexcept -> eastl::span<const std::pair<spdlog::level::level_enum, eastl::string>> {
            return m_Backtrace;
        }

        auto clear() noexcept -> void { m_Backtrace.clear(); }

    private:
        eastl::vector<std::pair<spdlog::level::level_enum, eastl::string>> m_Backtrace{};

        auto sink_it_(const spdlog::details::log_msg& msg) -> void override {
            spdlog::memory_buf_t buffer{};
            formatter_->format(msg, buffer);
            eastl::string message = {buffer.data(), buffer.size()};
            m_Backtrace.emplace_back(std::make_pair(msg.level, std::move(message)));
        }

        auto flush_() -> void override {}
    };
}
