// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"

#include "../platform/subsystem.hpp"

#include <mimalloc.h>
#include <bgfx/bgfx.h>
#include <bx/allocator.h>

#include "imgui/imgui_renderer.hpp"
#include "debugdraw/debugdraw.h"
#include "prelude.hpp"

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

        // load all shaders
        load_shader_registry("media/shaders", m_programs);
        passert(!m_programs.empty() && "No shaders found");
        for (auto&& [name, _] : m_programs) {
            log_info("Registered shader program '{}'", name);
        }
    }

    graphics_subsystem::~graphics_subsystem() {
        m_programs.clear();
        if (s_is_dd_init) {
            ddShutdown();
        }
        ImGuiEx::Destroy();
        bgfx::shutdown();
    }

    [[nodiscard]] static auto get_main_camera() -> entity {
        const auto& scene = scene::get_active();
        if (scene) [[likely]] {
            auto query = scene->filter<const c_transform, c_camera>();
            if (query.count() > 0) {
                return query.first();
            }
        }
        return entity::null();
    }

    static auto clear_main_render_target(float& width, float& height) -> void {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        width = static_cast<float>(w);
        height = static_cast<float>(h);
        bgfx::setViewRect(graphics_subsystem::k_scene_view, 0, 0, w, h);
        bgfx::setViewClear(graphics_subsystem::k_scene_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::touch(graphics_subsystem::k_scene_view);
    }

    static XMMATRIX m_mtx_view;
    static XMMATRIX m_mtx_proj;
    static XMMATRIX m_mtx_view_proj;

    static auto update_main_camera(float width, float height) -> void {
        entity main_cam = get_main_camera();
        if (!main_cam.is_valid() || !main_cam.is_alive()) [[unlikely]] {
            log_warn("No camera found in scene");
            return;
        }
        c_camera::active_camera = main_cam;
        const c_transform& transform = *main_cam.get<c_transform>();
        c_camera& cam = *main_cam.get_mut<c_camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        m_mtx_view = cam.compute_view(transform);
        m_mtx_proj = cam.compute_projection();
        m_mtx_view_proj = XMMatrixMultiply(m_mtx_view, m_mtx_proj);
        bgfx::setViewTransform(graphics_subsystem::k_scene_view, &m_mtx_view, &m_mtx_proj);
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        clear_main_render_target(m_width, m_height);
        update_main_camera(m_width, m_height);
        ImGuiEx::BeginFrame(m_width, m_height, k_imgui_view);
        s_is_draw_phase = true;
        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        s_is_draw_phase = false;
        ImGuiEx::EndFrame();
        bgfx::frame();
        c_camera::active_camera = entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {
        int w, h;
        glfwGetFramebufferSize(platform_subsystem::get_glfw_window(), &w, &h);
        bgfx::reset(w, h, m_reset_flags);
    }

    void graphics_subsystem::on_start(scene& scene) {
        entity camera = scene.entity();
        camera.add<c_transform>();
        camera.add<c_camera>();
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
