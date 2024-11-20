// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <functional>

namespace soliton {
    template <typename F>
    class exit_guard final {
    public:
        constexpr explicit exit_guard(F&& f) noexcept : m_f{std::forward<F>(f)} {}
        exit_guard(const exit_guard&) = delete;
        exit_guard(exit_guard&&) = delete;
        auto operator = (const exit_guard&) -> exit_guard& = delete;
        auto operator = (exit_guard&&) -> exit_guard& = delete;
        ~exit_guard() {
            eastl::invoke(m_f);
        }

    private:
        const F m_f;
    };
}
