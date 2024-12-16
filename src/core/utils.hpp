// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#define FMT_CONSTEVAL constexpr
#include <spdlog/spdlog.h>

#define log_info SPDLOG_INFO
#define log_warn SPDLOG_WARN
#define log_error SPDLOG_ERROR
#define log_critical SPDLOG_CRITICAL
#define print_sep() log_info("------------------------------------------------------------")

namespace soliton {
    [[noreturn]] extern auto panic_impl(std::string&& message) -> void;

    template <typename... Args>
    [[noreturn]] auto panic(const std::string_view message, Args&&... args) -> void {
        panic_impl(fmt::format(message, std::forward<Args>(args)...));
    }

    #define panic_assert(expr) \
        do { \
            if (!(expr)) { \
                ::soliton::panic("Assertion failed: {} in {}:{}", #expr, __FILE__, __LINE__); \
            } \
        } while (false)


    inline auto str_replace(eastl::string& str, const eastl::string_view from, const eastl::string_view to) -> bool {
        const std::size_t start_pos = str.find(from.data());
        if(start_pos == std::string::npos) return false;
        str.replace(start_pos, from.length(), to.data());
        return true;
    }
}
