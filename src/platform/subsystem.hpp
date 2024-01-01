// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include <GLFW/glfw3.h>

namespace platform {
    class platform_subsystem final : public subsystem {
    public:
        platform_subsystem();
        ~platform_subsystem() override;

        auto on_prepare() -> void override;
        HOTPROC auto on_pre_tick() -> bool override;

        [[nodiscard]] static auto get_glfw_window() -> GLFWwindow*;
        [[nodiscard]] static auto get_native_window() -> void*;
        [[nodiscard]] static auto get_native_display() -> void*;

        static constexpr const char* k_window_icon_file = "media/icons/logo.png";
    };
}
