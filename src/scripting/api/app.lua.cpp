// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"

#include "../../core/kernel.hpp"
#include "../../graphics/vulkancore/context.hpp"
#include "../../platform/platform_subsystem.hpp"
#include "../../graphics/graphics_subsystem.hpp"

#include <filesystem>
#include <infoware/infoware.hpp>
#include <nfd.hpp>

#include "../scripting_subsystem.hpp"

using graphics::graphics_subsystem;
using platform::platform_subsystem;
using namespace eastl::chrono;

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
    return platform_subsystem::get_main_window().is_focused();
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
    platform_subsystem::get_main_window().maximize();
}

LUA_INTEROP_API auto __lu_window_minimize() -> void {
    platform_subsystem::get_main_window().minimize();
}

LUA_INTEROP_API auto __lu_window_enter_fullscreen() -> void {
    platform_subsystem::get_main_window().enter_fullscreen();
}

LUA_INTEROP_API auto __lu_window_leave_fullscreen() -> void {
    platform_subsystem::get_main_window().exit_fullscreen();
}

LUA_INTEROP_API auto __lu_window_set_title(const char* const title) -> void {
    platform_subsystem::get_main_window().set_title(title);
}

LUA_INTEROP_API auto __lu_window_set_size(const int width, const int height) -> void {
    platform_subsystem::get_main_window().set_size({width, height});
}

LUA_INTEROP_API auto __lu_window_set_pos(const int x, const int y) -> void {
    platform_subsystem::get_main_window().set_pos({x, y});
}

LUA_INTEROP_API auto __lu_window_show() -> void {
    platform_subsystem::get_main_window().show();
}

LUA_INTEROP_API auto __lu_window_hide() -> void {
    platform_subsystem::get_main_window().hide();
}

LUA_INTEROP_API auto __lu_window_allow_resize(const bool allow) -> void {
    platform_subsystem::get_main_window().set_resizeable(allow);
}

LUA_INTEROP_API auto __lu_window_get_size() -> lua_vec2 {
    return lua_vec2{platform_subsystem::get_main_window().get_size()};
}

LUA_INTEROP_API auto __lu_window_get_framebuf_size() -> lua_vec2 {
    return lua_vec2{platform_subsystem::get_main_window().get_framebuffer_size()};
}

LUA_INTEROP_API auto __lu_window_get_pos() -> lua_vec2 {
   return lua_vec2{platform_subsystem::get_main_window().get_pos()};
}

LUA_INTEROP_API auto __lu_window_enable_cursor(const bool enable) -> void {
    platform_subsystem::get_main_window().set_cursor_mode(enable);
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

LUA_INTEROP_API auto __lu_app_get_subsystem_names(const char **data, const int size) -> std::uint32_t {
    const auto& kern = kernel::get();
    const auto& systems = kern.get_subsystems();
    panic_assert(size >= systems.size());
    for (std::size_t i = 0; i < systems.size(); ++i) {
        data[i] = systems[i]->name.c_str();
    }
    return systems.size();
}

LUA_INTEROP_API auto __lu_app_get_subsystem_pre_tick_times(double* const data, const int size) -> void {
    const auto& kern = kernel::get();
    const auto& systems = kern.get_subsystems();
    panic_assert(size >= systems.size());
    for (std::size_t i = 0; i < systems.size(); ++i) {
        data[i] = duration_cast<duration<double, eastl::milli>>(systems[i]->prev_pre_tick_time()).count();
    }
}

LUA_INTEROP_API auto __lu_app_get_subsystem_tick_times(double* const data, const int size) -> void {
    const auto& kern = kernel::get();
    const auto& systems = kern.get_subsystems();
    panic_assert(size >= systems.size());
    for (std::size_t i = 0; i < systems.size(); ++i) {
        data[i] = duration_cast<duration<double, eastl::milli>>(systems[i]->prev_tick_time()).count();
    }
}

LUA_INTEROP_API auto __lu_app_get_subsystem_post_tick_times(double* const data, const int size) -> void {
    const auto& kern = kernel::get();
    const auto& systems = kern.get_subsystems();
    panic_assert(size >= systems.size());
    for (std::size_t i = 0; i < systems.size(); ++i) {
        data[i] = duration_cast<duration<double, eastl::milli>>(systems[i]->prev_post_tick_time()).count();
    }
}

LUA_INTEROP_API auto __lu_app_copy_dir_recursive(const char* from, const char* to) -> bool {
    const std::filesystem::path from_path = from;
    const std::filesystem::path to_path = to;
    if (!std::filesystem::exists(from_path) || !std::filesystem::is_directory(from_path)) {
        return false;
    }
    if (!std::filesystem::exists(to_path)) {
        std::filesystem::create_directories(to_path);
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(from_path)) {
        const std::filesystem::path relative_path = entry.path().lexically_relative(from_path);
        const std::filesystem::path target_path = to_path / relative_path;
        if (std::filesystem::is_directory(entry)) {
            std::filesystem::create_directories(target_path);
        } else {
            std::filesystem::copy_file(entry, target_path, std::filesystem::copy_options::overwrite_existing);
        }
    }
    return true;
}
