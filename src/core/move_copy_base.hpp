// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

namespace soliton {
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
}
