// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../platform/platform_subsystem.hpp"

using platform::platform_subsystem;

#include <GLFW/glfw3.h>

LUA_INTEROP_API auto __lu_input_is_key_pressed(const int key_code) -> bool {
    return platform_subsystem::get_main_window().is_key_pressed(key_code);
}

LUA_INTEROP_API auto __lu_input_is_key_released(const int key_code) -> bool {
    return platform_subsystem::get_main_window().is_key_released(key_code);
}

LUA_INTEROP_API auto __lu_input_is_mouse_button_pressed(const int mb_code) -> bool {
    return platform_subsystem::get_main_window().is_mouse_button_pressed(mb_code);
}

LUA_INTEROP_API auto __lu_input_is_mouse_button_released(const int mb_code) -> bool {
    return platform_subsystem::get_main_window().is_mouse_button_released(mb_code);
}

LUA_INTEROP_API auto __lu_input_get_mouse_pos() -> lua_vec2 {
    const XMFLOAT2 pos = platform_subsystem::get_main_window().get_cursor_pos();
    return { pos.x, pos.y };
}

