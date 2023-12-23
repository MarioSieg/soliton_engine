// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "platform.hpp"

#include <array>
#include <string>
#include <span>
#include <vector>

#define FMT_CONSTEVAL constexpr

#include <spdlog/spdlog.h>

#define log_info SPDLOG_INFO
#define log_warn SPDLOG_WARN
#define log_error SPDLOG_ERROR
#define log_critical SPDLOG_CRITICAL
#define print_sep() log_info("------------------------------------------------------------")

class no_copy {
public:
    constexpr no_copy() = default;
    constexpr no_copy(const no_copy&) = delete;
    constexpr auto operator = (const no_copy&) -> no_copy& = delete;
    constexpr no_copy(no_copy&&) = default;
    constexpr auto operator = (no_copy&&) -> no_copy& = default;
    ~no_copy() = default;
};

class no_move {
public:
    constexpr no_move() = default;
    constexpr no_move(no_move&&) = delete;
    constexpr auto operator = (no_move&&) -> no_move& = delete;
    constexpr no_move(const no_move&) = default;
    constexpr auto operator = (const no_move&) -> no_move& = default;
    ~no_move() = default;
};

[[noreturn]] extern auto panic_impl(std::string&& message) -> void;

template <typename... Args>
[[noreturn]] auto panic(std::string_view message, Args&&... args) -> void {
    panic_impl(fmt::format(message, std::forward<Args>(args)...));
}

#define passert(expr) \
	do { \
		if (!(expr)) [[unlikely]] { \
			panic("Assertion failed: {} in {}:{}", #expr, __FILE__, __LINE__); \
		} \
	} while (false)
