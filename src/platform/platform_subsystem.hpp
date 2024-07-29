// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/subsystem.hpp"
#include <GLFW/glfw3.h>

namespace lu::platform {
    class platform_subsystem final : public subsystem {
    public:
        static constexpr bool k_use_mimalloc = true;

        platform_subsystem();
        ~platform_subsystem() override;

        auto on_prepare() -> void override;
        HOTPROC auto on_pre_tick() -> bool override;

        [[nodiscard]] static auto get_glfw_window() -> GLFWwindow*;
        static inline multicast_delegate<GLFWcursorposfun> s_cursor_pos_callbacks;
        static inline multicast_delegate<GLFWscrollfun> s_scroll_callbacks;
        static inline multicast_delegate<GLFWkeyfun> s_key_callbacks;
        static inline multicast_delegate<GLFWcharfun> s_char_callbacks;
        static inline multicast_delegate<GLFWmousebuttonfun> s_mouse_button_callbacks;
        static inline multicast_delegate<GLFWcursorenterfun> s_cursor_enter_callbacks;
        static inline multicast_delegate<GLFWframebuffersizefun> s_framebuffer_size_callbacks;
    };
}
