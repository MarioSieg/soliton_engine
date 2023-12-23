// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"
#include "../platform/subsystem.hpp"

#include <bgfx/bgfx.h>
#include <imgui.h>
#include "imgui/imgui_renderer.hpp"

using platform::platform_subsystem;

namespace graphics {
    graphics_subsystem::graphics_subsystem() : subsystem{"graphics"} {
        void* wnd = platform_subsystem::get_native_window();
        passert(wnd != nullptr);
        bgfx::Init init {};
        init.platformData.nwh = wnd;
        init.platformData.ndt = platform_subsystem::get_native_display();
        init.resolution.reset = m_reset_flags;
#if PLATFORM_WINDOWS
        init.type = bgfx::RendererType::Direct3D11;
#elif PLATFORM_LINUX
        init.type = bgfx::RendererType::OpenGL;
#elif PLATFORM_MACOS
        init.type = bgfx::RendererType::Metal;
#endif

        passert(bgfx::init(init));

        ImGuiEx::Create(platform_subsystem::get_glfw_window());
    }

    graphics_subsystem::~graphics_subsystem() {
        ImGuiEx::Destroy();
        bgfx::shutdown();
    }

    auto graphics_subsystem::on_pre_tick() -> bool {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        bgfx::setViewRect(0, 0, 0, w, h);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, ~0u, 1.0f, 0);
        bgfx::touch(0);
        ImGuiEx::BeginFrame(w, h, 0xff);
        ImGui::ShowDemoWindow();
        return true;
    }

    auto graphics_subsystem::on_post_tick() -> void {
        ImGuiEx::EndFrame();
        bgfx::frame();
    }

    auto graphics_subsystem::on_resize() -> void {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        bgfx::reset(w, h, m_reset_flags);
    }
}
