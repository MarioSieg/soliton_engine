// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "graphics_subsystem.hpp"

#include "../platform/platform_subsystem.hpp"

#include <execution>
#include <mimalloc.h>

#include "imgui/text_editor.hpp"
#include "imgui/implot.h"

using platform::platform_subsystem;

namespace graphics {
    using vkb::context;

    graphics_subsystem::graphics_subsystem() : subsystem{"Graphics"} {
        log_info("Initializing graphics subsystem");
        ImGui::CreateContext();
        ImPlot::CreateContext();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = 800.0f;
        io.DisplaySize.y = 600.0f;
        io.IniFilename = nullptr;

        GLFWwindow* window = platform_subsystem::get_glfw_window();
        context::s_instance = std::make_unique<context>(window); // Create vulkan context

        m_vs.emplace("triangle.vert");
        m_fs.emplace("triangle.frag");
    }

    graphics_subsystem::~graphics_subsystem() {
        m_fs.reset();
        m_vs.reset();
        context::s_instance.reset();

        ImPlot::DestroyContext();
        ImGui::DestroyContext();
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
    }

    HOTPROC auto graphics_subsystem::on_pre_tick() -> bool {
        ImGui::NewFrame();
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(context::s_instance->get_width());
        io.DisplaySize.y = static_cast<float>(context::s_instance->get_height());

        cmd_buf = context::s_instance->begin_frame(DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f});

        return true;
    }

    HOTPROC auto graphics_subsystem::on_post_tick() -> void {
        if (cmd_buf) [[likely]] {
            ImGui::Render();
            if (auto* dd = ImGui::GetDrawData(); dd) [[likely]] {
                context::s_instance->render_imgui(dd, cmd_buf);
            }
            context::s_instance->end_frame(cmd_buf);
        }

        c_camera::active_camera = entity::null();
    }

    auto graphics_subsystem::on_resize() -> void {

    }

    void graphics_subsystem::on_start(scene& scene) {
        entity camera = scene.spawn("MainCamera");
        camera.add<c_camera>();
    }
}
