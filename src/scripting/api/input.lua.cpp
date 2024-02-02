// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../platform/platform_subsystem.hpp"

using platform::platform_subsystem;

#include <GLFW/glfw3.h>

LUA_INTEROP_API auto __lu_input_is_key_pressed(const int key_code) -> bool {
    GLFWwindow* window = platform_subsystem::get_glfw_window();
    if (!window) [[unlikely]] {
        return false;
    }
    return glfwGetKey(window, key_code) == GLFW_PRESS;
}

LUA_INTEROP_API auto __lu_input_is_key_released(const int key_code) -> bool {
    GLFWwindow* window = platform_subsystem::get_glfw_window();
    if (!window) [[unlikely]] {
        return false;
    }
    return glfwGetKey(window, key_code) == GLFW_RELEASE;
}

LUA_INTEROP_API auto __lu_input_is_mouse_button_pressed(const int mb_code) -> bool {
    GLFWwindow* window = platform_subsystem::get_glfw_window();
    if (!window) [[unlikely]] {
        return false;
    }
    return glfwGetMouseButton(window, mb_code) == GLFW_PRESS;
}

LUA_INTEROP_API auto __lu_input_is_mouse_button_released(const int mb_code) -> bool {
    GLFWwindow* window = platform_subsystem::get_glfw_window();
    if (!window) [[unlikely]] {
        return false;
    }
    return glfwGetMouseButton(window, mb_code) == GLFW_RELEASE;
}

LUA_INTEROP_API auto __lu_input_get_mouse_pos() -> lua_vec2 {
    GLFWwindow* window = platform_subsystem::get_glfw_window();
    if (!window) [[unlikely]] {
        return lua_vec2{};
    }
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return lua_vec2{x, y};

}

