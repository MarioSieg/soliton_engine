// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "platform_subsystem.hpp"
#include "backend.hpp"
#include "machine_info.hpp"

#include "../scripting/system_variable.hpp"

namespace lu::platform {

    static const system_variable<std::int32_t> cv_default_width {"window.default_width", {1280}};
    static const system_variable<std::int32_t> cv_default_height {"window.default_height", {720}};
    static const system_variable<std::int32_t> cv_min_width {"window.min_width", {640}};
    static const system_variable<std::int32_t> cv_min_height {"window.min_height", {480}};
    static const system_variable<eastl::string> cv_window_icon {"window.icon", {"assets/icons/logo.png"}};

    static auto proxy_resize_hook(GLFWwindow* const window, const int w, const int h) -> void {
        panic_assert(window != nullptr);
        void* user = glfwGetWindowUserPointer(window);
        panic_assert(user);
        user = static_cast<class window*>(user)->usr;
        panic_assert(user);
        auto* sys = static_cast<platform_subsystem*>(user);
        panic_assert(sys->resize_hook);
        eastl::invoke(sys->resize_hook); // inform kernel about resize
        log_info("Resized window to {}x{}", w, h);
    }

    platform_subsystem::platform_subsystem() : subsystem{"Platform"} {
        backend::init();
        print_full_machine_info();
        m_main_window.emplace(
            "Lunam Engine " + get_version_string(),
            XMINT2 {cv_default_width(), cv_default_height()},
            false
        );
        m_main_window->set_size_limits(XMINT2 {cv_min_width(), cv_min_height()});
        m_main_window->set_icon(cv_window_icon());
        m_main_window->framebuffer_size_callbacks += &proxy_resize_hook;
        m_main_window->usr = this;
    }

    platform_subsystem::~platform_subsystem() {
        m_main_window.reset();
        backend::shutdown();
    }

    void platform_subsystem::on_prepare() {
        m_main_window->show();
        m_main_window->focus();
    }

    HOTPROC auto platform_subsystem::on_pre_tick() -> bool {
        backend::poll_events();
        return !m_main_window->should_close();
    }
}
