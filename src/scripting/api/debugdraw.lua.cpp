// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#include "../api_prelude.hpp"
#include "../../graphics/subsystem.hpp"
#include "../../graphics/debugdraw/debugdraw.h"

using graphics::graphics_subsystem;

static constinit std::optional<DebugDrawEncoder> s_encoder = std::nullopt;

LUA_INTEROP_API auto __lu_dd_begin() -> void {
    graphics_subsystem::init_debug_draw_lazy(); // initialize debug draw if not already initialized
    if (graphics_subsystem::is_draw_phase()) [[likely]] {
        s_encoder.emplace();
        s_encoder->begin(graphics_subsystem::k_scene_view);
    }
}

LUA_INTEROP_API auto __lu_dd_set_wireframe(const bool wireframe) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    s_encoder->setWireframe(wireframe);
}

LUA_INTEROP_API auto __lu_dd_set_color(const uint32_t abgr) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    s_encoder->setColor(abgr);
}

LUA_INTEROP_API auto __lu_dd_grid(const int axis, const lvec3 pos, const std::uint32_t size, const float step) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    bx::Vec3 axis_vec {0.0f, 0.0f, 0.0f};
    switch (axis) {
        case 0: axis_vec.x = 1.0f; break;
        case 1: axis_vec.y = 1.0f; break;
        case 2: axis_vec.z = 1.0f; break;
        default: break;
    }
    s_encoder->drawGrid(axis_vec, pos, size, step);
}

LUA_INTEROP_API auto __lu_dd_axis(const lvec3 pos, const float len, const int axis_highlight, const float thickness) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    Axis::Enum axis = Axis::Y;
    switch (axis_highlight) {
        case 0: axis = Axis::X; break;
        case 1: axis = Axis::Y; break;
        case 2: axis = Axis::Z; break;
        default: break;
    }
    s_encoder->drawAxis(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z), len, axis, thickness);
}

LUA_INTEROP_API auto __lu_dd_aabb(const lvec3 min, const lvec3 max) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    bx::Aabb aabb {min, max};
    s_encoder->draw(aabb);
}

LUA_INTEROP_API auto __lu_dd_sphere(const lvec3 center, const float radius) -> void {
    if (!s_encoder.has_value()) [[unlikely]] return;
    bx::Sphere sphere {center, radius};
    s_encoder->draw(sphere);
}

LUA_INTEROP_API auto __lu_dd_end()-> void  {
    if (graphics_subsystem::is_draw_phase() && s_encoder.has_value()) [[likely]] {
        s_encoder->end();
        s_encoder.reset();
    }
}
