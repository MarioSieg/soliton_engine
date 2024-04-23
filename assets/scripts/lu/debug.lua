----------------------------------------------------------------------------
-- Lunam Engine Debug Module
--
-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local C = ffi.C
local bit = require 'bit'
local bor, band, lshift = bit.bor, bit.band, bit.lshift

ffi.cdef[[
    void __lu_dd_begin(void);
    void __lu_dd_grid(lua_vec3 dims, double step, lua_vec3 color);
    void __lu_dd_gizmo_enable(bool enable);
    void __lu_dd_gizmo_manipulator(lua_entity_id id, int op, int mode, bool enable_snap, double snap_x, lua_vec3 color);
    void __lu_dd_enable_depth_test(bool enable);
    void __lu_dd_enable_fade(bool enable);
    void __lu_dd_set_fade_distance(double near, double far);
    void __lu_dd_draw_scene_with_aabbs(lua_vec3 color);
    void __lu_dd_draw_physics_debug(void);
    void __lu_dd_draw_native_log(bool scroll);
    void __lu_dd_draw_native_profiler(void);
]]

local Debug = {}

function Debug.start()
    C.__lu_dd_begin()
end

function Debug.drawGrid(dims, size, color)
    C.__lu_dd_grid(dims, size, color)
end

function Debug.gizmoEnable(enable)
    C.__lu_dd_gizmo_enable(enable)
end

function Debug.gizmoManipulator(entity, op, mode, enable_snap, snap_x, obb_color)
    C.__lu_dd_gizmo_manipulator(entity.id, op, mode, enable_snap, snap_x, obb_color)
end

function Debug.enableDepthTest(enable)
    C.__lu_dd_enable_depth_test(enable)
end

function Debug.enableFade(enable)
    C.__lu_dd_enable_fade(enable)
end

function Debug.setFadeDistance(near, far)
    C.__lu_dd_set_fade_distance(near, far)
end

function Debug.drawSceneDebug(color)
    C.__lu_dd_draw_scene_with_aabbs(color)
end

function Debug.drawPhysicsDebug()
    C.__lu_dd_draw_physics_debug()
end

function Debug.drawNativeLog(scroll)
    C.__lu_dd_draw_native_log(scroll)
end

function Debug.drawNativeProfiler()
    C.__lu_dd_draw_native_profiler()
end

return Debug
