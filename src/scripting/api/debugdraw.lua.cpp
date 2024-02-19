// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "_prelude.hpp"
#include "../../graphics/graphics_subsystem.hpp"
#include "../../graphics/imgui/ImGuizmo.h"

using graphics::graphics_subsystem;


LUA_INTEROP_API auto __lu_dd_begin() -> void {
    ImGuizmo::BeginFrame();
    const auto& io = ImGui::GetIO();
    ImGuizmo::SetRect(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Enable(true);
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
}

LUA_INTEROP_API auto __lu_dd_set_wireframe(const bool wireframe) -> void {

}

LUA_INTEROP_API auto __lu_dd_set_color(const uint32_t abgr) -> void {

}

LUA_INTEROP_API auto __lu_dd_grid(const int axis, const lua_vec3 pos, const float size) -> void {
    const auto* view = reinterpret_cast<const float*>(&graphics_subsystem::s_view_mtx);
    const auto* proj = reinterpret_cast<const float*>(&graphics_subsystem::s_proj_mtx);
    DirectX::XMFLOAT4X4A mtx {};
    DirectX::XMStoreFloat4x4A(&mtx, DirectX::XMMatrixTranslation(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z)));
    ImGuizmo::DrawGrid(view, proj, reinterpret_cast<const float*>(&mtx), 10);
}

LUA_INTEROP_API auto __lu_dd_axis(const lua_vec3 pos, const float len, const int axis_highlight, const float thickness) -> void {

}

LUA_INTEROP_API auto __lu_dd_aabb(const lua_vec3 min, const lua_vec3 max) -> void {

}

LUA_INTEROP_API auto __lu_dd_sphere(const lua_vec3 center, const float radius) -> void {

}

LUA_INTEROP_API auto __lu_dd_end() -> void  {

}

LUA_INTEROP_API auto __lu_dd_gizmo_manipulator(const flecs::id_t id, const int op, const int mode) -> void {
    const flecs::entity ent {scene::get_active(), id};
    if (auto* transform = ent.get_mut<com::transform>(); transform) [[likely]] {
        const auto* view = reinterpret_cast<const float*>(&graphics_subsystem::s_view_mtx);
        const auto* proj = reinterpret_cast<const float*>(&graphics_subsystem::s_proj_mtx);
        DirectX::XMFLOAT4X4A model_mtx {};
        DirectX::XMStoreFloat4x4A(&model_mtx, transform->compute_matrix());
        ImGuizmo::Manipulate(view, proj, static_cast<ImGuizmo::OPERATION>(op), static_cast<ImGuizmo::MODE>(mode), reinterpret_cast<float*>(&model_mtx), nullptr, nullptr);
        DirectX::XMVECTOR pos {}, rot {}, scale {};
        DirectX::XMMatrixDecompose(&scale, &rot, &pos, DirectX::XMLoadFloat4x4A(&model_mtx));
        DirectX::XMStoreFloat3(&transform->position, pos);
        DirectX::XMStoreFloat4(&transform->rotation, rot);
        DirectX::XMStoreFloat3(&transform->scale, scale);
    }
}
