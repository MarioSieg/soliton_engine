// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include "window.hpp"

namespace soliton::platform {
    class platform_subsystem final : public subsystem {
    public:
        platform_subsystem();
        ~platform_subsystem() override;

        auto on_start(scene&) -> void override;
        HOTPROC auto on_pre_tick() -> bool override;

        [[nodiscard]] static inline auto get_main_window() -> window& { return *m_main_window; }

    private:
        static inline eastl::optional<window> m_main_window {};
    };
}
