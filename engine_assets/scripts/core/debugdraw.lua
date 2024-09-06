-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- debugdraw Module - Functions for debugging drawing and visualization.
--- @module debugdraw
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    void __lu_dd_begin(void);
    void __lu_dd_draw_line(lua_vec3 from, lua_vec3 to, lua_vec3 color);
    void __lu_dd_draw_arrow(lua_vec3 from, lua_vec3 to, lua_vec3 color, double arrowhead_length);
    void __lu_dd_draw_arrow_dir(lua_vec3 from, lua_vec3 dir, lua_vec3 color, double arrowhead_length);
    void __lu_dd_draw_transform(lua_vec3 pos, lua_vec4 rot, double axis_len);
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

local debugdraw = {
    --- Gizmo operations flags for the gizmo manipulator function: debugdraw.gizmoManipulator().
    gizmo_operation = {
        translate_x = 0x01, -- Translate X
        translate_y = 0x02, -- Translate Y
        translate_z = 0x04, -- Translate Z
        rotate_x = 0x08, -- Rotate X
        rotate_y = 0x10, -- Rotate Y
        rotate_z = 0x20, -- Rotate Z
        rotate_screen = 0x40, -- Rotate Screen (camera)
        scale_x = 0x80, -- Scale X
        scale_y = 0x100, -- Scale Y
        scale_z = 0x200, -- Scale Z
        bounds = 0x400, -- Bounds (OBB)
        scale_x_u = 0x800, -- Scale X Uniform
        scale_y_u = 0x1000, -- Scale Y Uniform
        scale_z_u = 0x2000, -- Scale Z Uniform
        translate = 0, -- Translate all axes
        rotate = 0, -- Rotate all axes
        scale = 0, -- Scale all axes
        scale_u = 0, -- Scale all axes uniformly
        universal = 0 -- All operations combined
    },
    --- Gizmo modes for the gizmo manipulator function: debugdraw.gizmoManipulator().
    gizmo_mode = {
        local_space = 0, -- Local Space
        world_Space = 1 -- World Space
    }
}

debugdraw.gizmo_operation.translate = debugdraw.gizmo_operation.translate_x + debugdraw.gizmo_operation.translate_y + debugdraw.gizmo_operation.translate_z
debugdraw.gizmo_operation.rotate = debugdraw.gizmo_operation.rotate_x + debugdraw.gizmo_operation.rotate_y + debugdraw.gizmo_operation.rotate_z + debugdraw.gizmo_operation.rotate_screen
debugdraw.gizmo_operation.scale = debugdraw.gizmo_operation.scale_x + debugdraw.gizmo_operation.scale_y + debugdraw.gizmo_operation.scale_z
debugdraw.gizmo_operation.scale_u = debugdraw.gizmo_operation.scale_x_u + debugdraw.gizmo_operation.scale_y_u + debugdraw.gizmo_operation.scale_z_u
debugdraw.gizmo_operation.universal = debugdraw.gizmo_operation.translate + debugdraw.gizmo_operation.rotate + debugdraw.gizmo_operation.scale

--- Starts the debugdraw drawing. This function should be called at the beginning of the frame.
function debugdraw.start()
    cpp.__lu_dd_begin()
end

--- Draws a transform gizmo.
-- @tparam gmath.vec3 pos The position of the transform.
-- @tparam gmath.vec4 rot The rotation quaternion of the transform.
-- @tparam number axis_len The length of the axis.
function debugdraw.draw_transform(pos, rot, axis_len)
    cpp.__lu_dd_draw_transform(pos, rot, axis_len)
end

--- Draws a line in the scene.
-- @tparam gmath.vec3 start The start position of the line.
-- @tparam gmath.vec3 end The end position of the line.
-- @tparam gmath.vec3 color The color of the line.
function debugdraw.draw_line(from, to, color)
    cpp.__lu_dd_draw_line(from, to, color)
end

--- Draws an arrow in the scene.
-- @tparam gmath.vec3 start The start position of the arrow.
-- @tparam gmath.vec3 end The end position of the arrow.
-- @tparam gmath.vec3 color The color of the arrow.
function debugdraw.draw_arrow(from, to, color, arrowhead_length)
    cpp.__lu_dd_draw_arrow(from, to, color, arrowhead_length or 0.5)
end

--- Draws an arrow in the scene.
-- @tparam gmath.vec3 from The start position of the arrow.
-- @tparam gmath.vec3 dir The direction of the arrow.
-- @tparam gmath.vec3 color The color of the arrow.
function debugdraw.draw_arrow_dir(from, dir, color, arrowhead_length)
    cpp.__lu_dd_draw_arrow_dir(from, dir, color, arrowhead_length or 0.5)
end

--- Draws a grid in the scene.
-- @tparam gmath.vec3 dims The dimensions of the grid.
-- @tparam number size The size of the grid.
-- @tparam gmath.vec3 color The color of the grid.
function debugdraw.draw_grid(dims, size, color)
    cpp.__lu_dd_grid(dims, size, color)
end

--- Enables or disables the gizmo.
-- @tparam bool enable Whether to enable or disable the gizmo.
function debugdraw.enable_gizmo(enable)
    cpp.__lu_dd_gizmo_enable(enable)
end

--- Draws a gizmo manipulator for an entity with transform.
-- @tparam entity.entity entity The entity to draw the gizmo for.
-- @tparam debugdraw.gizmo_mode op The operation to perform.
-- @tparam debugdraw.GIZMO_MODE mode The mode of the gizmo.
-- @tparam boolean enable_snap Whether to enable snapping.
-- @tparam number snap_x The snap value.
-- @tparam gmath.vec3 obb_color The color of the OBB.
function debugdraw.draw_gizmo_manipulator(entity, op, mode, enable_snap, snap_x, obb_color)
    cpp.__lu_dd_gizmo_manipulator(entity._id, op, mode, enable_snap, snap_x, obb_color)
end

--- Enables or disables the depth test.
-- @tparam boolean enable Whether to enable or disable the depth test.
function debugdraw.enable_depth(enable)
    cpp.__lu_dd_enable_depth_test(enable)
end

--- Enables or disables the distance fade effect.
-- @tparam boolean enable Whether to enable or disable the fade effect.
function debugdraw.enable_fade(enable)
    cpp.__lu_dd_enable_fade(enable)
end

--- Sets the fade distance.
-- @tparam number near The near fade distance.
-- @tparam number far The far fade distance.
function debugdraw.set_fade_range(near, far)
    cpp.__lu_dd_set_fade_distance(near, far)
end

--- Draws all scene meshes scene with AABBs.
-- @tparam gmath.vec3 color The color of the AABBs.
function debugdraw.draw_all_aabbs(color)
    cpp.__lu_dd_draw_scene_with_aabbs(color)
end

--- Draws the physics debugdraw information, bounding boxes, etc.
function debugdraw.draw_all_physics_shapes()
    cpp.__lu_dd_draw_physics_debug()
end

--- Draws the native log. (Internally Used by the editor, do NOT call this function directly!)
function debugdraw.draw_native_log(scroll)
    cpp.__lu_dd_draw_native_log(scroll)
end

--- Draws the native profiler. (Internally Used by the editor, do NOT call this function directly!)
function debugdraw.draw_native_profiler()
    cpp.__lu_dd_draw_native_profiler()
end

return debugdraw
