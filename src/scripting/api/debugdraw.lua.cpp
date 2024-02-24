// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../graphics/graphics_subsystem.hpp"
#include "../../graphics/imgui/ImGuizmo.h"

using graphics::graphics_subsystem;


LUA_INTEROP_API auto __lu_dd_begin() -> void {
    ImGuizmo::BeginFrame();
    const auto& io = ImGui::GetIO();
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
}

LUA_INTEROP_API auto __lu_dd_grid(const lua_vec3 pos, const double step, const lua_vec3 color) -> void {
    graphics_subsystem::s_instance->get_debug_draw().draw_grid(pos, static_cast<float>(step), color);
}

LUA_INTEROP_API auto __lu_dd_gizmo_enable(const bool enable) -> void {
    ImGuizmo::Enable(enable);
}

LUA_INTEROP_API auto __lu_dd_gizmo_manipulator(const flecs::id_t id, const int op, const int mode, const bool enable_snap, const double snap_x, const lua_vec3 color) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (!ent.has<const com::transform>()) [[unlikely]] {
        return;
    }
    auto* transform = ent.get_mut<com::transform>();
    const auto* view = reinterpret_cast<const float*>(&graphics_subsystem::s_view_mtx);
    const auto* proj = reinterpret_cast<const float*>(&graphics_subsystem::s_proj_mtx);
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
    DirectX::XMStoreFloat3(&transform->position, pos);
    DirectX::XMStoreFloat4(&transform->rotation, rot);
    DirectX::XMStoreFloat3(&transform->scale, scale);
    if (auto* renderer = ent.get<com::mesh_renderer>(); renderer) {
        for (const auto* mesh : renderer->meshes) {
            if (mesh) [[likely]] {
                DirectX::BoundingOrientedBox obb {};
                DirectX::BoundingOrientedBox::CreateFromBoundingBox(obb, mesh->get_aabb());
                const auto model = DirectX::XMLoadFloat4x4A(&model_mtx);
                graphics_subsystem::s_instance->get_debug_draw().draw_obb(obb, model, color);
            }
        }
    }
}

LUA_INTEROP_API auto __lu_dd_enable_depth_test(const bool enable) -> void {
    graphics_subsystem::s_instance->get_debug_draw().set_depth_test(enable);
}

LUA_INTEROP_API auto __lu_dd_enable_fade(const bool enable) -> void {
    graphics_subsystem::s_instance->get_debug_draw().set_distance_fade_enable(enable);
}

LUA_INTEROP_API auto __lu_dd_set_fade_distance(const double $near, const double $far) -> void {
    graphics_subsystem::s_instance->get_debug_draw().set_fade_start(static_cast<float>($near));
    graphics_subsystem::s_instance->get_debug_draw().set_fade_end(static_cast<float>($far));
}
