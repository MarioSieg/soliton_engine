// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

#include <GLFW/glfw3.h>

namespace lu::platform {
    class window final : public no_copy, public no_move {
    public:
        window(
            const eastl::string& title,
            XMINT2 size,
            bool is_visible
        );
        ~window();

        multicast_delegate<GLFWcursorposfun> cursor_pos_callbacks {};
        multicast_delegate<GLFWscrollfun> scroll_callbacks {};
        multicast_delegate<GLFWkeyfun> key_callbacks {};
        multicast_delegate<GLFWcharfun> char_callbacks {};
        multicast_delegate<GLFWmousebuttonfun> mouse_button_callbacks {};
        multicast_delegate<GLFWcursorenterfun> cursor_enter_callbacks {};
        multicast_delegate<GLFWframebuffersizefun> framebuffer_size_callbacks {};

        auto maximize() -> void;
        auto minimize() -> void;
        auto enter_fullscreen() -> void;
        auto exit_fullscreen() -> void;
        auto toggle_fullscreen() -> void;
        [[nodiscard]] auto is_maximized() -> bool;
        [[nodiscard]] auto is_minimized() -> bool;
        [[nodiscard]] auto is_focused() -> bool;
        [[nodiscard]] auto is_fullscreen() -> bool;
        auto set_size(XMINT2 size) -> void;
        auto set_pos(XMINT2 pos) -> void;
        auto set_title(const eastl::string& title) -> void;
        auto set_size_limits(XMINT2 min, XMINT2 max) -> void;
        auto set_size_limits(XMINT2 min) -> void;
        auto set_icon(const eastl::string& texture_path) -> void;
        auto set_resizeable(bool is_resizable) -> void;
        auto set_cursor_mode(bool enabled) -> void;
        auto focus() -> void;
        auto show() -> void;
        auto hide() -> void;
        auto close() -> void;
        [[nodiscard]] auto should_close() const -> bool;
        [[nodiscard]] auto get_size() const -> XMINT2;
        [[nodiscard]] auto get_framebuffer_size() const -> XMINT2;
        [[nodiscard]] auto get_cursor_pos() const -> XMFLOAT2;
        [[nodiscard]] auto get_pos() const -> XMINT2;
        [[nodiscard]] auto get_content_scale() const -> XMFLOAT2;
        [[nodiscard]] auto is_key_pressed(std::int32_t key_code) const -> bool;
        [[nodiscard]] auto is_key_released(std::int32_t key_code) const -> bool;
        [[nodiscard]] auto is_mouse_button_pressed(std::int32_t mb_code) const -> bool;
        [[nodiscard]] auto is_mouse_button_released(std::int32_t mb_code) const -> bool;
        [[nodiscard]] auto operator *() const noexcept -> GLFWwindow*;

    private:
        GLFWwindow* m_window = nullptr;
        bool m_is_fullscreen = false;
        XMINT2 m_fullscreen_prev_pos {};
        XMINT2 m_fullscreen_prev_size {};
    };
}
