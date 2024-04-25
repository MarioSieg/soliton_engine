-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- Debug Module - Functions for debugging drawing and visualization.
-- @module Debug
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

local Debug = {
    --- Gizmo operations flags for the gizmo manipulator function: Debug.gizmoManipulator().
    GIZMO_OPERATIONS = {
        TRANSLATE_X = 0x01, -- Translate X
        TRANSLATE_Y = 0x02, -- Translate Y
        TRANSLATE_Z = 0x04, -- Translate Z
        ROTATE_X = 0x08, -- Rotate X
        ROTATE_Y = 0x10, -- Rotate Y
        ROTATE_Z = 0x20, -- Rotate Z
        ROTATE_SCREEN = 0x40, -- Rotate Screen (Camera)
        SCALE_X = 0x80, -- Scale X
        SCALE_Y = 0x100, -- Scale Y
        SCALE_Z = 0x200, -- Scale Z
        BOUNDS = 0x400, -- Bounds (OBB)
        SCALE_XU = 0x800, -- Scale X Uniform
        SCALE_YU = 0x1000, -- Scale Y Uniform
        SCALE_ZU = 0x2000, -- Scale Z Uniform
        TRANSLATE = 0, -- Translate all axes
        ROTATE = 0, -- Rotate all axes
        SCALE = 0, -- Scale all axes
        SCALEU = 0, -- Scale all axes uniformly
        UNIVERSAL = 0 -- All operations combined
    },
    --- Gizmo modes for the gizmo manipulator function: Debug.gizmoManipulator().
    GIZMO_MODE = {
        LOCAL = 0, -- Local Space
        WORLD = 1 -- World Space
    }
}

Debug.GIZMO_OPERATIONS.TRANSLATE = Debug.GIZMO_OPERATIONS.TRANSLATE_X
    + Debug.GIZMO_OPERATIONS.TRANSLATE_Y
    + Debug.GIZMO_OPERATIONS.TRANSLATE_Z
Debug.GIZMO_OPERATIONS.ROTATE = Debug.GIZMO_OPERATIONS.ROTATE_X
    + Debug.GIZMO_OPERATIONS.ROTATE_Y
    + Debug.GIZMO_OPERATIONS.ROTATE_Z
    + Debug.GIZMO_OPERATIONS.ROTATE_SCREEN
Debug.GIZMO_OPERATIONS.SCALE = Debug.GIZMO_OPERATIONS.SCALE_X
    + Debug.GIZMO_OPERATIONS.SCALE_Y
    + Debug.GIZMO_OPERATIONS.SCALE_Z
Debug.GIZMO_OPERATIONS.SCALEU = Debug.GIZMO_OPERATIONS.SCALE_XU
    + Debug.GIZMO_OPERATIONS.SCALE_YU
    + Debug.GIZMO_OPERATIONS.SCALE_ZU
Debug.GIZMO_OPERATIONS.UNIVERSAL = Debug.GIZMO_OPERATIONS.TRANSLATE
    + Debug.GIZMO_OPERATIONS.ROTATE
    + Debug.GIZMO_OPERATIONS.SCALEU

--- Starts the debug drawing. This function should be called at the beginning of the frame.
function Debug.start()
    C.__lu_dd_begin()
end

--- Draws a grid in the scene.
-- @tparam Math.Vec3 dims The dimensions of the grid.
-- @tparam number size The size of the grid.
-- @tparam Math.Vec3 color The color of the grid.
function Debug.drawGrid(dims, size, color)
    C.__lu_dd_grid(dims, size, color)
end

--- Enables or disables the gizmo.
-- @tparam bool enable Whether to enable or disable the gizmo.
function Debug.gizmoEnable(enable)
    C.__lu_dd_gizmo_enable(enable)
end

--- Draws a gizmo manipulator for an entity with transform.
-- @tparam Entity.Entity entity The entity to draw the gizmo for.
-- @tparam Debug.GIZMO_OPERATIONS op The operation to perform.
-- @tparam Debug.GIZMO_MODE mode The mode of the gizmo.
-- @tparam boolean enable_snap Whether to enable snapping.
-- @tparam number snap_x The snap value.
-- @tparam Math.Vec3 obb_color The color of the OBB.
function Debug.gizmoManipulator(entity, op, mode, enable_snap, snap_x, obb_color)
    C.__lu_dd_gizmo_manipulator(entity.id, op, mode, enable_snap, snap_x, obb_color)
end

--- Enables or disables the depth test.
-- @tparam boolean enable Whether to enable or disable the depth test.
function Debug.enableDepthTest(enable)
    C.__lu_dd_enable_depth_test(enable)
end

--- Enables or disables the distance fade effect.
-- @tparam boolean enable Whether to enable or disable the fade effect.
function Debug.enableFade(enable)
    C.__lu_dd_enable_fade(enable)
end

--- Sets the fade distance.
-- @tparam number near The near fade distance.
-- @tparam number far The far fade distance.
function Debug.setFadeDistance(near, far)
    C.__lu_dd_set_fade_distance(near, far)
end

--- Draws all scene meshes scene with AABBs.
-- @tparam Math.Vec3 color The color of the AABBs.
function Debug.drawSceneDebug(color)
    C.__lu_dd_draw_scene_with_aabbs(color)
end

--- Draws the physics debug information, bounding boxes, etc.
function Debug.drawPhysicsDebug()
    C.__lu_dd_draw_physics_debug()
end

--- Draws the native log. (Internally Used by the editor, do NOT call this function directly!)
function Debug.drawNativeLog(scroll)
    C.__lu_dd_draw_native_log(scroll)
end

--- Draws the native profiler. (Internally Used by the editor, do NOT call this function directly!)
function Debug.drawNativeProfiler()
    C.__lu_dd_draw_native_profiler()
end

return Debug
