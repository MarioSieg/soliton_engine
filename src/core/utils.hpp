// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#define FMT_CONSTEVAL constexpr
#include <spdlog/spdlog.h>

#define log_info SPDLOG_INFO
#define log_warn SPDLOG_WARN
#define log_error SPDLOG_ERROR
#define log_critical SPDLOG_CRITICAL
#define print_sep() log_info("------------------------------------------------------------")

namespace lu {
    [[noreturn]] extern auto panic_impl(std::string&& message) -> void;

    template <typename... Args>
    [[noreturn]] auto panic(const std::string_view message, Args&&... args) -> void {
        panic_impl(fmt::format(message, std::forward<Args>(args)...));
    }

    #define passert(expr) \
        do { \
            if (!(expr)) [[unlikely]] { \
                ::lu::panic("Assertion failed: {} in {}:{}", #expr, __FILE__, __LINE__); \
            } \
        } while (false)
}
