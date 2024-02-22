-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local C = ffi.C
local bit = require 'bit'
local bor, band, lshift = bit.bor, bit.band, bit.lshift

ffi.cdef[[
    void __lu_dd_begin(void);
    void __lu_dd_grid(lua_vec3 dims, double step, lua_vec3 color);
    void __lu_dd_gizmo_enable(bool enable);
    void __lu_dd_gizmo_manipulator(lua_entity_id id, int op, int mode, bool enable_snap, double snap_x, lua_vec3 color);
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

return Debug
