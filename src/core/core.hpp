// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "platform.hpp"

#include <array>
#include <string>
#include <vector>

#define FMT_CONSTEVAL constexpr

#include <spdlog/spdlog.h>

template <typename... Ts>
[[nodiscard]] inline auto Format(const fmt::format_string<Ts...>& formatString, Ts&&... args) -> std::string {
    return fmt::format(formatString, std::forward<Ts>(args)...);
}

#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

class NoCopy {
public:
    constexpr NoCopy() = default;
    constexpr NoCopy(const NoCopy&) = delete;
    constexpr auto operator = (const NoCopy&) -> NoCopy& = delete;
    constexpr NoCopy(NoCopy&&) = default;
    constexpr auto operator = (NoCopy&&) -> NoCopy& = default;
    ~NoCopy() = default;
};

class NoMove {
public:
    constexpr NoMove() = default;
    constexpr NoMove(NoMove&&) = delete;
    constexpr auto operator = (NoMove&&) -> NoMove& = delete;
    constexpr NoMove(const NoMove&) = default;
    constexpr auto operator = (const NoMove&) -> NoMove& = default;
    ~NoMove() = default;
};

[[noreturn]] extern auto PanicImpl(std::string&& message) -> void;

template <typename... Args>
[[noreturn]] auto Panic(std::string_view message, Args&&... args) -> void {
    PanicImpl(Format(message, std::forward<Args>(args)...));
}

#define Assert(expr) \
	do { \
		if (!(expr)) [[unlikely]] { \
			Panic("Assertion failed: {} in {}:{}", #expr, __FILE__, __LINE__); \
		} \
	} while (false)
