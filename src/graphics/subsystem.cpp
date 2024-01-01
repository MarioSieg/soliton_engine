// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"
#include "../platform/subsystem.hpp"

#include <mimalloc.h>
#include <bgfx/bgfx.h>
#include <bx/allocator.h>

#include "imgui/imgui_renderer.hpp"
#include "debugdraw/debugdraw.h"

using platform::platform_subsystem;

namespace graphics {
    class allocator final : public bx::AllocatorI {
    public:
        auto realloc(void* p, size_t size, size_t align, const char* _filePath, uint32_t _line) -> void* override;
    };

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");
        void* wnd = platform_subsystem::get_native_window();
        passert(wnd != nullptr);
        static allocator g_bgfx_alloc;
        bgfx::Init init {};
        init.allocator = &g_bgfx_alloc;
        init.platformData.nwh = wnd;
        init.platformData.ndt = platform_subsystem::get_native_display();
        init.resolution.reset = m_reset_flags;
#if PLATFORM_WINDOWS
        init.type = bgfx::RendererType::Direct3D11;
#elif PLATFORM_LINUX
        init.type = bgfx::RendererType::Vulkan;
#elif PLATFORM_MACOS
        init.type = bgfx::RendererType::Metal;
#endif

        passert(bgfx::init(init));

        ImGuiEx::Create(platform_subsystem::get_glfw_window());
    }

    graphics_subsystem::~graphics_subsystem() {
        if (s_is_dd_init) {
            ddShutdown();
        }
        ImGuiEx::Destroy();
        bgfx::shutdown();
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        m_width = static_cast<float>(w);
        m_height = static_cast<float>(h);
        bgfx::setViewRect(k_scene_view, 0, 0, w, h);
        bgfx::setViewClear(k_scene_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::touch(k_scene_view);
        ImGuiEx::BeginFrame(w, h, k_imgui_view);
        s_is_draw_phase = true;
        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {

        const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };
        const bx::Vec3 eye = { 0.0f, 2.0f, -5.0f };

        float view[16];
        bx::mtxLookAt(view, eye, at);

        float proj[16];
        bx::mtxProj(proj, 60.0f, m_width / m_height, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(k_scene_view, view, proj);

        s_is_draw_phase = false;
        ImGuiEx::EndFrame();
        bgfx::frame();
    }

    auto graphics_subsystem::on_resize() -> void {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        bgfx::reset(w, h, m_reset_flags);
    }

    auto graphics_subsystem::init_debug_draw_lazy() -> void {
        if (!s_is_dd_init) [[unlikely]] {
            ddInit();
            s_is_dd_init = true;
        }
    }

    static constexpr std::size_t k_natural_align = 8;

    auto allocator::realloc(void* p, std::size_t size, std::size_t align, const char*, std::uint32_t) -> void* {
        if (0 == size) {
            if (nullptr != p) {
                if (k_natural_align >= align) {
                    mi_free(p);
                    return nullptr;
                }
                mi_free_aligned(p, align);
            }
            return nullptr;
        }
        if (nullptr == p) {
            if (k_natural_align >= align) {
                return mi_malloc(size);
            }
            return mi_malloc_aligned(size, align);
        }
        if (k_natural_align >= align) {
            return mi_realloc(p, size);
        }
        return mi_realloc_aligned(p, size, align);
    }
}
