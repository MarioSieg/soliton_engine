// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "buffered_sink.hpp"

namespace lu {
    auto buffered_sink::sink_it_(const spdlog::details::log_msg& msg) -> void {
        spdlog::memory_buf_t buffer{};
        formatter_->format(msg, buffer);
        eastl::string message = {buffer.data(), buffer.size()};
        m_history.emplace_back(eastl::make_pair(msg.level, std::move(message)));
    }
}
