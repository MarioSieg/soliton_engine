// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../platform/subsystem.hpp"

using platform::platform_subsystem;

typedef struct { int v[2]; } __lu_ivec2;

LUA_INTEROP_API void __lu_panic(const char* msg) {
    panic(msg ? msg : "unknown error");
}

LUA_INTEROP_API uint32_t __lu_ffi_cookie(void)  {
    return 0xfefec0c0;
}

LUA_INTEROP_API void __lu_window_maximize(void) {
    glfwMaximizeWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API void __lu_window_minimize(void) {
    glfwIconifyWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API void __lu_window_enter_fullscreen(void) {

}

LUA_INTEROP_API void __lu_window_leave_fullscreen(void) {

}

LUA_INTEROP_API void __lu_window_set_title(const char* title) {
    glfwSetWindowTitle(platform_subsystem::get_glfw_window(), title);
}

LUA_INTEROP_API void __lu_window_set_size(int width, int height) {
    glfwSetWindowSize(platform_subsystem::get_glfw_window(), width, height);
}

LUA_INTEROP_API void __lu_window_set_pos(int x, int y) {
    glfwSetWindowPos(platform_subsystem::get_glfw_window(), x, y);
}

LUA_INTEROP_API void __lu_window_show(void) {
    glfwShowWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API void __lu_window_hide(void) {
    glfwHideWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API void __lu_window_allow_resize(bool allow) {
    glfwSetWindowAttrib(platform_subsystem::get_glfw_window(), GLFW_RESIZABLE, allow);
}

LUA_INTEROP_API __lu_ivec2 __lu_window_get_size(void) {
    int width, height;
    glfwGetWindowSize(platform_subsystem::get_glfw_window(), &width, &height);
    return __lu_ivec2 { .v = { width, height } };
}

LUA_INTEROP_API __lu_ivec2 __lu_window_get_pos(void) {
    int x, y;
    glfwGetWindowPos(platform_subsystem::get_glfw_window(), &x, &y);
    return __lu_ivec2 { .v = { x, y } };
}
