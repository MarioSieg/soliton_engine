// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../core/buffered_sink.hpp"
#include "../../graphics/graphics_subsystem.hpp"
#include "../../graphics/imgui/ImGuiProfilerRenderer.h"
#include "../../graphics/imgui/ImGuizmo.h"
#include "../../physics/physics_subsystem.hpp"


using graphics::graphics_subsystem;

[[nodiscard]] static inline auto dd() noexcept -> graphics::debugdraw& { return graphics_subsystem::get().get_debug_draw(); }

LUA_INTEROP_API auto __lu_dd_begin() -> void {
    ImGuizmo::BeginFrame();
    const auto& io = ImGui::GetIO();
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
}

LUA_INTEROP_API auto __lu_dd_grid(const lua_vec3 pos, const double step, const lua_vec3 color) -> void {
    dd().draw_grid(pos, static_cast<float>(step), color);
}

LUA_INTEROP_API auto __lu_dd_gizmo_enable(const bool enable) -> void {
    ImGuizmo::Enable(enable);
}

LUA_INTEROP_API auto __lu_dd_gizmo_manipulator(const lua_entity_id id, const int op, const int mode, const bool enable_snap, const double snap_x, const lua_vec3 color) -> void {
    const flecs::entity ent {scene::get_active(), std::bit_cast<flecs::id_t>(id)};
    if (!ent.has<const com::transform>()) [[unlikely]] {
        return;
    }
    auto* transform = ent.get_mut<com::transform>();
    const auto* view = reinterpret_cast<const float*>(&graphics_subsystem::get_view_mtx());
    const auto* proj = reinterpret_cast<const float*>(&graphics_subsystem::get_proj_mtx());
    DirectX::XMFLOAT4X4A model_mtx;
    DirectX::XMStoreFloat4x4A(&model_mtx, transform->compute_matrix());
    DirectX::XMFLOAT3A snap;
    DirectX::XMStoreFloat3A(&snap, DirectX::XMVectorReplicate(static_cast<float>(snap_x)));
    ImGuizmo::Manipulate(
        view,
        proj,
        static_cast<ImGuizmo::OPERATION>(op),
        static_cast<ImGuizmo::MODE>(mode),
        reinterpret_cast<float*>(&model_mtx),
        nullptr,
        enable_snap ? reinterpret_cast<const float*>(&snap) : nullptr
    );
    DirectX::XMVECTOR pos {}, rot {}, scale {};
    DirectX::XMMatrixDecompose(&scale, &rot, &pos, DirectX::XMLoadFloat4x4A(&model_mtx));
    DirectX::XMStoreFloat4(&transform->position, pos);
    DirectX::XMStoreFloat4(&transform->rotation, rot);
    DirectX::XMStoreFloat4(&transform->scale, scale);
    if (const auto* const renderer = ent.get<com::mesh_renderer>(); renderer) {
        for (const auto* const mesh : renderer->meshes) {
            if (mesh) [[likely]] {
                DirectX::BoundingOrientedBox obb {};
                DirectX::BoundingOrientedBox::CreateFromBoundingBox(obb, mesh->get_aabb());
                const DirectX::XMMATRIX model = DirectX::XMLoadFloat4x4A(&model_mtx);
                dd().draw_obb(obb, model, color);
            }
        }
    }
}

LUA_INTEROP_API auto __lu_dd_enable_depth_test(const bool enable) -> void {
    dd().set_depth_test(enable);
}

LUA_INTEROP_API auto __lu_dd_enable_fade(const bool enable) -> void {
    dd().set_distance_fade_enable(enable);
}

LUA_INTEROP_API auto __lu_dd_set_fade_distance(const double $near, const double $far) -> void {
    dd().set_fade_start(static_cast<float>($near));
    dd().set_fade_end(static_cast<float>($far));
}

LUA_INTEROP_API auto __lu_dd_draw_scene_with_aabbs(const lua_vec3 color) -> void {
    const DirectX::XMFLOAT3 ccolor = color;
    dd().begin_batch();
    scene::get_active().filter<const com::transform, const com::mesh_renderer>().each([&ccolor](const com::transform& transform, const com::mesh_renderer& renderer) {
        for (const auto* mesh : renderer.meshes) {
           if (mesh) [[likely]] {
               DirectX::BoundingOrientedBox obb {};
               DirectX::BoundingOrientedBox::CreateFromBoundingBox(obb, mesh->get_aabb());
               const DirectX::XMMATRIX model = transform.compute_matrix();
               dd().draw_obb(obb, model, ccolor);
               dd().draw_transform(model, 1.0f);
           }
       }
    });
    dd().end_batch();
}

LUA_INTEROP_API auto __lu_dd_draw_physics_debug() -> void {
    physics::debug_renderer::begin();
    static constexpr auto draw_settings = [] {
        JPH::BodyManager::DrawSettings settings {};
        settings.mDrawVelocity = true;
        settings.mDrawWorldTransform = true;
        settings.mDrawBoundingBox = true;
        settings.mDrawMassAndInertia = true;
        settings.mDrawShape = true;
        settings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MaterialColor;
        return settings;
    }();
    physics::physics_subsystem::get_physics_system().DrawBodies(draw_settings, &physics::physics_subsystem::get_debug_renderer());
    physics::debug_renderer::end();
}

LUA_INTEROP_API auto __lu_dd_draw_native_log(const bool scroll) -> void {
    std::shared_ptr<spdlog::logger> logger = spdlog::get("engine");
    const auto& sinks = logger->sinks();
    if (sinks.empty()) [[unlikely]] {
        return;
    }
    const auto& sink = sinks.front();
    auto* buffered = static_cast<buffered_sink*>(&*sink);
    if (!buffered) [[unlikely]] {
        return;
    }
    const std::span<const std::pair<spdlog::level::level_enum, eastl::string>> logs = buffered->get();
    using namespace ImGui;
    const float footer = GetStyle().ItemSpacing.y + GetFrameHeightWithSpacing();
    const auto id = static_cast<ImGuiID>(std::bit_cast<std::uintptr_t>(logs.data()) ^ 0xfefefefe);
    if (BeginChild(id, { .0F, -footer }, false)) {
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 { 4.F, 1.F });
        ImGuiListClipper clipper {};
        clipper.Begin(static_cast<int>(logs.size()), GetTextLineHeightWithSpacing());
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                using enum spdlog::level::level_enum;
                const auto& [level, message] = logs[i];
                Separator();
                std::uint32_t color;
                switch (level) {
                    case err: color = 0xff9091f3; break;
                    case warn: color = 0xff76acf1; break;
                    default: color = 0xffeeeeee; break;
                }
                PushStyleColor(ImGuiCol_Text, color);
                TextUnformatted(message.c_str());
                PopStyleColor();
            }
        }
        clipper.End();
        if (scroll) {
            SetScrollHereY(1.0);
        }
        PopStyleVar();
    }
    EndChild();
}

LUA_INTEROP_API auto __lu_dd_draw_native_profiler() -> void {
    panic("not implemented");
}
