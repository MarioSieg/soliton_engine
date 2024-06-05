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

[[nodiscard]] consteval auto make_version(const std::uint8_t major, const std::uint8_t minor) -> std::uint32_t { return (static_cast<std::uint32_t>(major)<<8)|minor; }
[[nodiscard]] consteval auto major_version(const std::uint32_t v) -> std::uint8_t { return (v>>8)&0xff; }
[[nodiscard]] consteval auto minor_version(const std::uint32_t v) -> std::uint8_t { return v&0xff; }

constexpr std::uint32_t k_lunam_engine_v = make_version(0, 2); // current engine version (must be known at compile time and we don't use patches yet)

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

template <typename F>
class exit_guard final {
public:
    constexpr explicit exit_guard(F&& f) noexcept : m_f{std::forward<F>(f)} {}
    exit_guard(const exit_guard&) = delete;
    exit_guard(exit_guard&&) = delete;
    auto operator = (const exit_guard&) -> exit_guard& = delete;
    auto operator = (exit_guard&&) -> exit_guard& = delete;
    ~exit_guard() {
        std::invoke(m_f);
    }

private:
    const F m_f;
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

#define USE_MIMALLOC 1
