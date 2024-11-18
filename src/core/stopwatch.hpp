// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include <chrono>

#include "utils.hpp"

namespace soliton {
    /**
    * @brief A stopwatch class that uses eastl::chrono for timekeeping.
    * @tparam Clock
    */
    template<typename Clock = eastl::chrono::high_resolution_clock>
    class stopwatch final {
    public:
        [[nodiscard]] inline auto stamp() const noexcept -> auto { return m_stamp; }

        template<typename Dur = typename Clock::duration>
        [[nodiscard]] inline auto elapsed() const -> Dur {
            return eastl::chrono::duration_cast<Dur>(Clock::now() - m_stamp);
        }

        template <typename S = float> requires std::is_floating_point_v<S>
        [[nodiscard]] inline auto elapsed_secs() const -> S {
            return eastl::chrono::duration_cast<eastl::chrono::duration<S>>(elapsed<>()).count();
        }

        inline auto restart() -> void { m_stamp = Clock::now(); }

    private:
        typename Clock::time_point m_stamp {Clock::now()};
    };
}
