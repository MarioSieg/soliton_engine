// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "window.hpp"
#include "backend.hpp"

#include "../assetmgr/assetmgr.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace lu::platform {
    static auto glfw_cursor_pos_callback(GLFWwindow* const window, double x, double y) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.cursor_pos_callbacks(window, x, y);
    }

    static auto glfw_scroll_callback(GLFWwindow* const window, const double x, const double y) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.scroll_callbacks(window, x, y);
    }

    static auto glfw_key_callback(GLFWwindow* const window, const int key, const int scancode, const int action, const int mods) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.key_callbacks(window, key, scancode, action, mods);
    }

    static auto glfw_char_callback(GLFWwindow* const window, const unsigned codepoint) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.char_callbacks(window, codepoint);
    }

    static auto glfw_mouse_button_callback(GLFWwindow* const window, const int button, const int action, const int mods) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.mouse_button_callbacks(window, button, action, mods);
    }

    static auto glfw_cursor_enter_callback(GLFWwindow* const window, const int entered) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.cursor_enter_callbacks(window, entered);
    }

    static auto glfw_framebuffer_size_callback(GLFWwindow* const window, const int w, const int h) -> void {
        passert(window != nullptr);
        passert(glfwGetWindowUserPointer(window) != nullptr);
        auto& self = *static_cast<struct window*>(glfwGetWindowUserPointer(window));
        self.framebuffer_size_callbacks(window, w, h);
    }

    window::window(
        const eastl::string& title,
        const XMINT2 size,
        const bool is_visible
    ) {
        passert(backend::is_online());

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, is_visible ? GLFW_TRUE : GLFW_FALSE);
        m_window = glfwCreateWindow(
            std::max(size.x, 800),
            std::max(size.y, 600),
            !title.empty() ? title.c_str() : "Lunam Engine Window",
            nullptr,
            nullptr
        );
        if (!m_window) [[unlikely]]
            panic("Failed to create window with title: {}, Dimensions: {}", title.c_str(), size);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetCursorPosCallback(m_window, &glfw_cursor_pos_callback);
        glfwSetScrollCallback(m_window, &glfw_scroll_callback);
        glfwSetKeyCallback(m_window, &glfw_key_callback);
        glfwSetCharCallback(m_window, &glfw_char_callback);
        glfwSetMouseButtonCallback(m_window, &glfw_mouse_button_callback);
        glfwSetCursorEnterCallback(m_window, &glfw_cursor_enter_callback);
        glfwSetFramebufferSizeCallback(m_window, &glfw_framebuffer_size_callback);
    }

    auto window::maximize() -> void {
        glfwMaximizeWindow(m_window);
    }

    auto window::minimize() -> void {
        glfwIconifyWindow(m_window);
    }

    auto window::is_maximized() -> bool {
        return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    auto window::is_minimized() -> bool {
        return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    auto window::is_focused() -> bool {
        return glfwGetWindowAttrib(m_window, GLFW_FOCUSED) == GLFW_TRUE;
    }

    auto window::set_size(const XMINT2 size) -> void {
        glfwSetWindowSize(m_window, std::max(size.x, 100), std::max(size.y, 100));
    }

    auto window::set_pos(const XMINT2 pos) -> void {
        glfwSetWindowPos(m_window, std::max(pos.x, 0), std::max(pos.y, 0));
    }

    auto window::set_title(const eastl::string& title) -> void {
        glfwSetWindowTitle(m_window, title.c_str());
    }

    auto window::set_size_limits(const XMINT2 min, const XMINT2 max) -> void {
        glfwSetWindowSizeLimits(m_window, std::max(min.x, 100), std::max(min.y, 100), std::max(max.x, 200), std::max(max.y, 200));
    }

    auto window::set_size_limits(const XMINT2 min) -> void {
        glfwSetWindowSizeLimits(m_window, std::max(min.x, 100), std::max(min.y, 100), GLFW_DONT_CARE, GLFW_DONT_CARE);
    }

    auto window::focus() -> void {
        glfwFocusWindow(m_window);
    }

    auto window::show() -> void {
        glfwShowWindow(m_window);
    }

    auto window::hide() -> void {
        glfwHideWindow(m_window);
    }

    auto window::close() -> void {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    auto window::should_close() const -> bool {
        return glfwWindowShouldClose(m_window) == GLFW_TRUE;
    }

    auto window::get_size() const -> XMINT2 {
        XMINT2 size;
        glfwGetWindowSize(m_window, &size.x, &size.y);
        return size;
    }

    auto window::get_framebuffer_size() const -> XMINT2 {
        XMINT2 size;
        glfwGetFramebufferSize(m_window, &size.x, &size.y);
        return size;
    }

    auto window::get_cursor_pos() const -> XMFLOAT2 {
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        return { static_cast<float>(x), static_cast<float>(y) };
    }

    auto window::operator *() const noexcept -> GLFWwindow* {
        return m_window;
    }

    window::~window() {
        glfwDestroyWindow(m_window);
    }

    auto window::set_icon(const eastl::string& texture_path) -> void {
        if constexpr (PLATFORM_OSX) { // Cocoa - regular windows do not have icons on macOS
            log_warn("Setting window icon is not supported on macOS");
            return;
        }

        eastl::vector<std::byte> pixel_buf {};

        bool success = false;
        assetmgr::with_primary_accessor_lock([&](assetmgr::asset_accessor& acc) {
            success = acc.load_bin_file(texture_path.c_str(), pixel_buf);
        });

        if (!success) [[unlikely]] {
            log_error("Failed to load window icon: '{}'", texture_path);
            return;
        }

        int w, h;
        stbi_uc* const pixels = stbi_load_from_memory(
            reinterpret_cast<const std::uint8_t*>(pixel_buf.data()),
            static_cast<int>(pixel_buf.size()),
            &w,
            &h,
            nullptr,
            STBI_rgb_alpha
        );

        if (!pixels) [[unlikely]] {
            log_error("Failed to load window icon: '{}'", texture_path);
            return;
        }

        const GLFWimage icon {
            .width = w,
            .height = h,
            .pixels = pixels
        };
        glfwSetWindowIcon(m_window, 1, &icon);

        stbi_image_free(pixels);
    }

    auto window::get_pos() const -> XMINT2 {
        XMINT2 pos;
        glfwGetWindowPos(m_window, &pos.x, &pos.y);
        return pos;
    }

    auto window::get_content_scale() const -> XMFLOAT2 {
        float x, y;
        glfwGetWindowContentScale(m_window, &x, &y);
        return { x, y };
    }

    auto window::is_key_pressed(const std::int32_t key_code) const -> bool {
        return glfwGetKey(m_window, key_code) == GLFW_PRESS;
    }

    auto window::is_key_released(const std::int32_t key_code) const -> bool {
        return glfwGetKey(m_window, key_code) == GLFW_RELEASE;
    }

    auto window::is_mouse_button_pressed(const std::int32_t mb_code) const -> bool {
        return glfwGetMouseButton(m_window, mb_code) == GLFW_PRESS;
    }

    auto window::is_mouse_button_released(const std::int32_t mb_code) const -> bool {
        return glfwGetMouseButton(m_window, mb_code) == GLFW_RELEASE;
    }

    auto window::is_fullscreen() -> bool {
        return m_is_fullscreen;
    }

    auto window::enter_fullscreen() -> void {
        if (m_is_fullscreen) return;

        GLFWmonitor* const primary_mon = glfwGetPrimaryMonitor();
        if (!primary_mon) [[unlikely]] {
            log_warn("Failed to fetch primary monitor");
            return;
        }

        const GLFWvidmode* primary_mode = glfwGetVideoMode(primary_mon);
        if (!primary_mode) [[unlikely]] {
            log_warn("Failed to fetch primary video mode");
            return;
        }

        m_fullscreen_prev_pos = get_pos();
        m_fullscreen_prev_size = get_size();

        glfwSetWindowMonitor(m_window, primary_mon, 0, 0, primary_mode->width, primary_mode->height, GLFW_DONT_CARE);

        m_is_fullscreen = true;
    }

    auto window::exit_fullscreen() -> void {
        if (!m_is_fullscreen) return;

        glfwSetWindowMonitor(m_window, nullptr, m_fullscreen_prev_pos.x, m_fullscreen_prev_pos.y, m_fullscreen_prev_size.x, m_fullscreen_prev_size.y, GLFW_DONT_CARE);

        m_is_fullscreen = false;
    }

    auto window::toggle_fullscreen() -> void {
        if (m_is_fullscreen) exit_fullscreen();
        else enter_fullscreen();
    }

    auto window::set_resizeable(const bool is_resizable) -> void {
        glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, is_resizable ? GLFW_TRUE : GLFW_FALSE);
    }

    auto window::set_cursor_mode(const bool enabled) -> void {
        glfwSetInputMode(m_window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}
