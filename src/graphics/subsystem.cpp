// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "subsystem.hpp"

#include "../platform/subsystem.hpp"

#include <execution>
#include <mimalloc.h>
#include <bgfx/bgfx.h>

#include "imgui/imgui_renderer.hpp"
#include "debugdraw/debugdraw.h"
#include "prelude.hpp"
#include "../rendercore/bgfx/3rdparty/dear-imgui/imgui.h"

using platform::platform_subsystem;

namespace graphics {
    class callback_hook final : public bgfx::CallbackI {
        auto fatal(const char* filePath, std::uint16_t line, bgfx::Fatal::Enum code, const char* str) -> void override;
        auto traceVargs(const char* filePath, std::uint16_t line, const char* format, va_list argList) -> void override;
		auto profilerBegin(const char* name, std::uint32_t abgr, const char* filePath, std::uint16_t line) -> void override;
		auto profilerBeginLiteral(const char* name, std::uint32_t abgr, const char* filePath, std::uint16_t line) -> void override;
		auto profilerEnd() -> void override;
		auto cacheReadSize(std::uint64_t id) -> std::uint32_t override;
		auto cacheRead(std::uint64_t id, void* data, std::uint32_t size)  -> bool override;
		auto cacheWrite(std::uint64_t id, const void* data, std::uint32_t size) -> void override;
		auto screenShot(const char* _filePath, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, const void* data, std::uint32_t size, bool yflip) -> void override;
		auto captureBegin(std::uint32_t width, std::uint32_t height, std::uint32_t pitch, bgfx::TextureFormat::Enum format, bool yflip) -> void override;
		auto captureEnd() -> void override;
		auto captureFrame(const void* _data, std::uint32_t size) -> void override;
    };

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");
        void* wnd = platform_subsystem::get_native_window();
        passert(wnd != nullptr);
        static allocator g_bgfx_alloc;
        static callback_hook g_bgfx_callback;
        bgfx::Init init {};
        init.allocator = &g_bgfx_alloc;
        init.platformData.nwh = wnd;
        init.platformData.ndt = platform_subsystem::get_native_display();
        init.resolution.reset = m_reset_flags;
        init.callback = &g_bgfx_callback;
#if PLATFORM_WINDOWS
        init.type = bgfx::RendererType::Direct3D11;
#elif PLATFORM_LINUX
        init.type = bgfx::RendererType::Vulkan;
#elif PLATFORM_MACOS
        init.type = bgfx::RendererType::Metal;
#endif

        passert(bgfx::init(init));

        //bgfx::reset(init.resolution.width, init.resolution.height, m_reset_flags);

        ImGuiEx::Create(platform_subsystem::get_glfw_window(), 20.0f);

        // load all shaders
        load_shader_registry("media/compiledshaders", m_programs);
        passert(!m_programs.empty() && "No shaders found");
        for (auto&& [name, _] : m_programs) {
            log_info("Registered shader program '{}'", name);
        }

        m_sampler = handle{bgfx::createUniform("s_sampler", bgfx::UniformType::Sampler)};
        m_mesh.emplace("media/meshes/orb.obj");
        auto load_mat = [](const std::string& name) {
            auto load_tex = [&name](std::string&& type, bool srgb = false) {
                return std::make_unique<texture>("media/textures/"+name+"/"+type+".png", true, srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE);
            };
            return material{
                load_tex("albedo", true),
                load_tex("normal"),
                load_tex("metallic"),
                load_tex("roughness"),
                load_tex("ao"),
            };
        };
        static constexpr std::array<std::string_view, 5> mat_names = {
            "plastic",
            "wall",
            "gold",
            "rusted_iron",
            "grass"
        };
        std::vector<std::pair<std::string, std::optional<material>>> results {};
        results.reserve(mat_names.size());
        m_materials.reserve(mat_names.size());
        for (auto&& name : mat_names)
            results.emplace_back(std::make_pair(std::move(name), std::nullopt));
        std::for_each(std::execution::par_unseq, results.begin(), results.end(), [load_mat](auto& p) {
            p.second = load_mat(p.first);
        });
        for (auto&& [name, mat] : results) {
            m_materials.emplace_back(std::move(*mat));
        }
        m_pbr.emplace();
    }

    graphics_subsystem::~graphics_subsystem() {
        m_pbr.reset();
        m_sampler.reset();
        m_materials.clear();
        m_mesh.reset();
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
    static c_transform m_camera_transform;

    static auto update_main_camera(float width, float height) -> void {
        entity main_cam = get_main_camera();
        if (!main_cam.is_valid() || !main_cam.is_alive()) [[unlikely]] {
            log_warn("No camera found in scene");
            return;
        }
        c_camera::active_camera = main_cam;
        m_camera_transform = *main_cam.get<c_transform>();
        c_camera& cam = *main_cam.get_mut<c_camera>();
        if (cam.auto_viewport) {
            cam.viewport.x = width;
            cam.viewport.y = height;
        }
        m_mtx_view = cam.compute_view(m_camera_transform);
        m_mtx_proj = cam.compute_projection();
        m_mtx_view_proj = XMMatrixMultiply(m_mtx_view, m_mtx_proj);
        bgfx::setViewTransform(graphics_subsystem::k_scene_view, &m_mtx_view, &m_mtx_proj);
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        clear_main_render_target(m_width, m_height);
        update_main_camera(m_width, m_height);
        ImGuiEx::BeginFrame(m_width, m_height, k_imgui_view);
        s_is_draw_phase = true;

        m_pbr->set_camera_pos(m_camera_transform.position);

        const auto& program = m_programs["pbr"];

        static constexpr std::uint64_t state =
                BGFX_STATE_WRITE_RGB
                | BGFX_STATE_WRITE_Z
                | BGFX_STATE_DEPTH_TEST_LESS
                | BGFX_STATE_CULL_CCW
                | BGFX_STATE_MSAA;

        float lim = m_materials.size();
        int i = 0;
        if (ImGui::Begin("Trololo")) {
            float r = m_pbr->get_roughness();
            ImGui::SliderFloat("Roughness", &r, 0.01f, 0.99f);
            m_pbr->set_roughness(r);
            float m = m_pbr->get_metalness();
            ImGui::SliderFloat("Metalness", &m, 0.01f, 0.99f);
            m_pbr->set_metalness(m);
        }
        ImGui::End();
        for (float y = 0.0f; y < lim; y += 1.0f) {
            for (float x = 0.0f; x < lim; x += 1.0f) {
                XMMATRIX mtx = XMMatrixMultiply(XMMatrixTranslation(x * 2.0f, 0.0f, y * 2.0f), XMMatrixRotationY(XMConvertToRadians(180.0f)));
                bgfx::setTransform(&mtx);
                bgfx::setState(state);
                material& mat = m_materials[i++ % m_materials.size()];
                m_pbr->submit_material_data(
                    mtx,
                    *mat.m_albedo->handle,
                    *mat.m_normal->handle,
                    *mat.m_metallic->handle,
                    *mat.m_roughness->handle,
                    *mat.m_ao->handle
                );
                m_mesh->render(graphics_subsystem::k_scene_view, *program);
            }
        }
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
        entity camera = scene.spawn("MainCamera");
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

    auto callback_hook::fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) -> void {
        panic("renderer fatal error: {}:{}: {} ({})", filePath, line, str, code);
    }

    auto callback_hook::traceVargs(const char* filePath, std::uint16_t line, const char* format, va_list argList) -> void {
        char buf[8192];
        std::vsnprintf(buf, sizeof(buf), format, argList);
        buf[std::strlen(buf)-1] = '\0';
        log_info("{}:{}: {}", filePath, line, buf);
    }

    auto callback_hook::profilerBegin(const char* name, std::uint32_t abgr, const char* filePath,
        std::uint16_t line) -> void {
    }

    auto callback_hook::profilerBeginLiteral(const char* name, std::uint32_t abgr, const char* filePath,
        std::uint16_t line) -> void {
    }

    auto callback_hook::profilerEnd() -> void {
    }

    auto callback_hook::cacheReadSize(std::uint64_t id) -> std::uint32_t {
        return 0;
    }

    auto callback_hook::cacheRead(std::uint64_t id, void* data, std::uint32_t size) -> bool {
        return false;
    }

    auto callback_hook::cacheWrite(std::uint64_t id, const void* data, std::uint32_t size) -> void {
    }

    auto callback_hook::screenShot(const char* _filePath, std::uint32_t width, std::uint32_t height,
        std::uint32_t pitch, const void* data, std::uint32_t size, bool yflip) -> void {
    }

    auto callback_hook::captureBegin(std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
        bgfx::TextureFormat::Enum format, bool yflip) -> void {
    }

    auto callback_hook::captureEnd() -> void {
    }

    auto callback_hook::captureFrame(const void* _data, std::uint32_t _size) -> void {
    }
}
