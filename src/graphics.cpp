// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics.hpp"

#include <atomic>
#include <bgfx/bgfx.h>

static constinit std::atomic_bool s_Initialized;
static constinit GLFWwindow* s_Window;
static constinit std::uint32_t s_ResetFlags = BGFX_RESET_SRGB_BACKBUFFER;

auto Graphics::InitGraphics(GLFWwindow* window, void* nativeWindow) -> void {
    Assert(!s_Initialized.load(std::memory_order_seq_cst));
    Assert(window != nullptr && nativeWindow != nullptr);
    s_Window = window;

    bgfx::Init init {};
    init.platformData.nwh = nativeWindow;
    init.resolution.reset = s_ResetFlags;
#if PLATFORM_WINDOWS
    init.type = bgfx::RendererType::Direct3D11;
#elif PLATFORM_LINUX
    init.type = bgfx::RendererType::OpenGL;
#elif PLATFORM_MACOS
    init.type = bgfx::RendererType::Metal;
#endif

    Assert(bgfx::init(init));

    s_Initialized.store(true, std::memory_order_seq_cst);
}

auto Graphics::BeginFrame() -> void {
    Assert(s_Initialized.load(std::memory_order_seq_cst));
    int w, h;
    glfwGetFramebufferSize(s_Window, &w, &h);
    bgfx::setViewRect(0, 0, 0, w, h);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, ~0u, 1.0f, 0);
    bgfx::touch(0);
}

auto Graphics::EndFrame() -> void {
    Assert(s_Initialized.load(std::memory_order_seq_cst));
    bgfx::frame();
}

auto Graphics::OnResize() -> void {
    int w, h;
    glfwGetFramebufferSize(s_Window, &w, &h);
    bgfx::reset(w, h, s_ResetFlags);
}

auto Graphics::ShutdownGraphics() -> void {
    Assert(s_Initialized.load(std::memory_order_seq_cst));
    bgfx::shutdown();
    s_Initialized.store(false, std::memory_order_seq_cst);
}
