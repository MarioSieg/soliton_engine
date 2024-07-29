// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <cstddef>
#include <functional>

namespace lu {
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
