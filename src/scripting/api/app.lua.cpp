// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../core/kernel.hpp"
#include "../../vulkancore/context.hpp"
#include "../../platform/platform_subsystem.hpp"

#include <infoware/infoware.hpp>

using platform::platform_subsystem;

LUA_INTEROP_API auto __lu_panic(const char* const msg) -> void {
    panic(msg ? msg : "unknown error");
}

LUA_INTEROP_API auto __lu_ffi_cookie() -> std::uint32_t  {
    return 0xfefec0c0;
}

LUA_INTEROP_API auto __lu_window_maximize() -> void {
    glfwMaximizeWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API auto __lu_window_minimize() -> void {
    glfwIconifyWindow(platform_subsystem::get_glfw_window());
}

static constinit int s_window_pos_x = 0;
static constinit int s_window_pos_y = 0;
static constinit int s_window_width = 1280;
static constinit int s_window_height = 720;

LUA_INTEROP_API auto __lu_window_enter_fullscreen() -> void {
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    if (!mon) [[unlikely]] { return; }
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    if (!mode) [[unlikely]] { return; }
    glfwGetWindowPos(platform_subsystem::get_glfw_window(), &s_window_pos_x, &s_window_pos_y);
    glfwGetWindowSize(platform_subsystem::get_glfw_window(), &s_window_width, &s_window_height);
    glfwSetWindowMonitor(platform_subsystem::get_glfw_window(), mon, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
}

LUA_INTEROP_API auto __lu_window_leave_fullscreen() -> void {
    s_window_width = std::max(s_window_width, 1280);
    s_window_height = std::max(s_window_height, 720);
    glfwSetWindowMonitor(platform_subsystem::get_glfw_window(), nullptr,
        s_window_pos_x, s_window_pos_y, s_window_width, s_window_height, GLFW_DONT_CARE);
}

LUA_INTEROP_API auto __lu_window_set_title(const char* const title) -> void {
    glfwSetWindowTitle(platform_subsystem::get_glfw_window(), title);
}

LUA_INTEROP_API auto __lu_window_set_size(const int width, const int height) -> void {
    glfwSetWindowSize(platform_subsystem::get_glfw_window(), width, height);
}

LUA_INTEROP_API auto __lu_window_set_pos(const int x, const int y) -> void {
    glfwSetWindowPos(platform_subsystem::get_glfw_window(), x, y);
}

LUA_INTEROP_API auto __lu_window_show() -> void {
    glfwShowWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API auto __lu_window_hide() -> void {
    glfwHideWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API auto __lu_window_allow_resize(const bool allow) -> void {
    glfwSetWindowAttrib(platform_subsystem::get_glfw_window(), GLFW_RESIZABLE, allow);
}

LUA_INTEROP_API auto __lu_window_get_size() -> lua_vec2 {
    int width, height;
    glfwGetWindowSize(platform_subsystem::get_glfw_window(), &width, &height);
    return lua_vec2 { static_cast<float>(width), static_cast<float>(height) };
}

LUA_INTEROP_API auto __lu_window_get_framebuf_size() -> lua_vec2 {
    int width, height;
    glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &width, &height);
    return lua_vec2 { static_cast<float>(width), static_cast<float>(height) };
}

LUA_INTEROP_API auto __lu_window_get_pos() -> lua_vec2 {
    int x, y;
    glfwGetWindowPos(platform_subsystem::get_glfw_window(), &x, &y);
    return lua_vec2 { static_cast<float>(x), static_cast<float>(y) };
}

LUA_INTEROP_API auto __lu_app_exit() -> void {
    kernel::get().request_exit();
}

static std::string tmp;

LUA_INTEROP_API auto __lu_app_host_get_cpu_name() -> const char* {
    tmp = iware::cpu::model_name();
    return tmp.c_str();
}

LUA_INTEROP_API auto __lu_app_host_get_gpu_name() -> const char* {
    return vkb::context::s_instance->get_device().get_physical_device_props().deviceName;
}

LUA_INTEROP_API auto __lu_app_host_get_gapi_name() -> const char* {
    const std::uint32_t api = vkb::context::s_instance->get_device().get_physical_device_props().apiVersion;
    const std::uint32_t major = VK_API_VERSION_MAJOR(api);
    const std::uint32_t minor = VK_API_VERSION_MINOR(api);
    const std::uint32_t patch = VK_API_VERSION_PATCH(api);
    tmp = fmt::format("Vulkan v.{}.{}.{}", major, minor, patch);
    return tmp.c_str();
}

