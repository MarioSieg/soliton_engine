// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

#include "../../core/kernel.hpp"
#include "../../graphics/vulkancore/context.hpp"
#include "../../platform/platform_subsystem.hpp"
#include "../../graphics/graphics_subsystem.hpp"

#include <infoware/infoware.hpp>
#include <nfd.hpp>

#include "../scripting_subsystem.hpp"

using graphics::graphics_subsystem;
using platform::platform_subsystem;

static constinit int s_window_pos_x = 0;
static constinit int s_window_pos_y = 0;
static constinit int s_window_width = 1280;
static constinit int s_window_height = 720;
static eastl::string s_tmp_proxy;

LUA_INTEROP_API auto __lu_panic(const char* const msg) -> void {
    panic(msg ? msg : "unknown error");
}

LUA_INTEROP_API auto __lu_ffi_cookie() -> std::uint32_t  {
    return 0xfefec0c0;
}

LUA_INTEROP_API auto __lu_engine_version() -> std::uint32_t {
    return k_lunam_engine_version;
}

LUA_INTEROP_API auto __lu_app_is_focused() -> bool {
    return glfwGetWindowAttrib(platform_subsystem::get_glfw_window(), GLFW_FOCUSED) == GLFW_TRUE;
}

LUA_INTEROP_API auto __lu_app_is_ui_hovered() -> bool {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

LUA_INTEROP_API auto __lu_app_hot_reload_ui(const bool render_wireframe) -> void {
    graphics_subsystem::get().get_noesis_context().reload_ui(render_wireframe);
}

LUA_INTEROP_API auto __lu_app_hot_reload_shaders() -> void {
    graphics_subsystem::get().hot_reload_pipelines();
}

LUA_INTEROP_API auto __lu_window_maximize() -> void {
    glfwMaximizeWindow(platform_subsystem::get_glfw_window());
}

LUA_INTEROP_API auto __lu_window_minimize() -> void {
    glfwIconifyWindow(platform_subsystem::get_glfw_window());
}

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

LUA_INTEROP_API auto __lu_window_enable_cursor(const bool enable) -> void {
    glfwSetInputMode(platform_subsystem::get_glfw_window(), GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

LUA_INTEROP_API auto __lu_app_exit() -> void {
    kernel::get().request_exit();
}

LUA_INTEROP_API auto __lu_app_get_draw_calls() -> std::uint32_t {
    return graphics_subsystem::get_num_draw_calls();
}

LUA_INTEROP_API auto __lu_app_get_draw_verts() -> std::uint32_t {
    return graphics_subsystem::get_num_draw_verts();
}

LUA_INTEROP_API auto __lu_app_host_get_cpu_name() -> const char* {
    s_tmp_proxy = iware::cpu::model_name().c_str();
    return s_tmp_proxy.c_str();
}

LUA_INTEROP_API auto __lu_app_host_get_gpu_name() -> const char* {
    return vkb::ctx().get_device().get_physical_device_props().deviceName;
}

LUA_INTEROP_API auto __lu_app_host_get_gapi_name() -> const char* {
    const volatile std::uint32_t api = vkb::ctx().get_device().get_physical_device_props().apiVersion;
    const std::uint32_t major = VK_API_VERSION_MAJOR(api);
    const std::uint32_t minor = VK_API_VERSION_MINOR(api);
    const std::uint32_t patch = VK_API_VERSION_PATCH(api);
    s_tmp_proxy = fmt::format("Vulkan v.{}.{}.{}", major, minor, patch).c_str();
    return s_tmp_proxy.c_str();
}

LUA_INTEROP_API auto __lu_app_host_get_num_cpus() -> std::uint32_t {
    static const std::uint32_t s_num_cpus = std::max(1u, std::thread::hardware_concurrency());
    return s_num_cpus;
}

LUA_INTEROP_API auto __lu_app_open_file_dialog(const char *file_type, const char* filters, const char* default_path) -> const char* {
    nfdchar_t *out;
    const nfdfilteritem_t filter = {file_type, filters};
    const nfdresult_t result = NFD_OpenDialog(&out, &filter, 1, default_path);
    if (result == NFD_OKAY) [[likely]] {
        s_tmp_proxy = out;
        NFD_FreePath(out);
        std::ranges::replace(s_tmp_proxy, '\\', '/');
        return s_tmp_proxy.c_str();
    }
    return "";
}

LUA_INTEROP_API auto __lu_app_open_folder_dialog(const char* default_path) -> const char* {
    nfdchar_t *out;
    const nfdresult_t result = NFD_PickFolder(&out, default_path);
    if (result == NFD_OKAY) [[likely]] {
        s_tmp_proxy = out;
        NFD_FreePath(out);
        std::ranges::replace(s_tmp_proxy, '\\', '/');
        s_tmp_proxy.push_back('/');
        return s_tmp_proxy.c_str();
    }
    return "";
}
