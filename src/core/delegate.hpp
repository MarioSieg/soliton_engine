// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <functional>
#include <list>
#include <vector>

#include "utils.hpp"

namespace lu {
    [[noreturn]] extern auto panic_impl(std::string&& message) -> void;

    template <typename T>
    struct multicast_delegate final {
    public:
        inline auto operator += (T&& d) noexcept -> void {
            passert(!m_is_locked);
            m_delegates.emplace_back(std::forward<T>(d));
        }
        inline auto operator -= (const T& d) noexcept -> void {
            passert(!m_is_locked);
            m_delegates.remove(d);
        }

        template <typename... Args>
        inline auto operator () (Args&&... args) noexcept(std::is_nothrow_invocable_v<T, Args...>) -> void {
            ++m_invocations;
            for (auto&& f : m_delegates)
                std::invoke(f, std::forward<Args>(args)...);
        }

        inline auto clear() noexcept -> void {
            passert(!m_is_locked);
            m_delegates.clear();
            m_invocations = 0;
        }

        [[nodiscard]] inline auto empty() const noexcept -> bool { return m_delegates.empty(); }
        [[nodiscard]] inline auto listeners() const noexcept -> const std::list<T>& { return m_delegates; }
        [[nodiscard]] inline auto invocations() const noexcept -> std::uint64_t { return m_invocations; }
        [[nodiscard]] inline auto is_locked() const noexcept -> bool { return m_is_locked; }
        inline auto set_locked(const bool locked) noexcept -> void { m_is_locked = locked; }

    private:
        std::list<T> m_delegates {};
        std::uint64_t m_invocations {};
        bool m_is_locked {};
    };
}
