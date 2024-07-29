// Copyright (c) 2022 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <array>
#include <algorithm>
#include <bit>
#include <memory>
#include <string>
#include <limits>
#include <span>
#include <vector>

#include "platform.hpp"
#include "crc32.hpp"
#include "delegate.hpp"
#include "utils.hpp"

#include <ankerl/unordered_dense.h>

#define USE_MIMALLOC 1

namespace lu {
    [[nodiscard]] consteval auto make_version(const std::uint8_t major, const std::uint8_t minor) -> std::uint32_t { return (static_cast<std::uint32_t>(major)<<8)|minor; }
    [[nodiscard]] consteval auto major_version(const std::uint32_t v) -> std::uint8_t { return (v>>8)&0xff; }
    [[nodiscard]] consteval auto minor_version(const std::uint32_t v) -> std::uint8_t { return v&0xff; }

    constexpr std::uint32_t k_lunam_engine_v = make_version(0, 3); // current engine version (must be known at compile time and we don't use patches yet)

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

    constexpr auto hash_merge(std::size_t lhs, const std::size_t rhs) noexcept -> std::size_t {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
        return lhs;
    }

    template <typename T>
    constexpr auto hash_merge(std::size_t& seed, const T& v) noexcept -> void {
        std::hash<T> hasher {};
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}
