-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bor, band, lshift = bit.bor, bit.band, bit.lshift

ffi.cdef[[
    void __lu_dd_begin(void);
    void __lu_dd_set_wireframe(bool wireframe);
    void __lu_dd_set_color(uint32_t abgr);
    void __lu_dd_grid(int axis, lua_vec3 pos, float size);
    void __lu_dd_axis(lua_vec3 pos, float len, int axis_highlight, float thickness);
    void __lu_dd_aabb(lua_vec3 min, lua_vec3 max);
    void __lu_dd_sphere(lua_vec3 center, float radius);
    void __lu_dd_end(void);
    void __lu_dd_gizmo_manipulator(lua_entity_id id);
]]

local C = ffi.C
local istype = ffi.istype

local Debug = {
    AXIS = {
        X = 0,
        Y = 1,
        Z = 2
    }
}

function Debug.start()
    C.__lu_dd_begin()
end

function Debug.setWireframe(isWireframe)
    C.__lu_dd_set_wireframe(isWireframe)
end

function Debug.setColor(rgb)
    local r = bit.band(rgb.x*255.0, 0xff) -- normalized [0,1] to u8[0,0xff]
    local g = bit.band(rgb.y*255.0, 0xff)
    local b = bit.band(rgb.z*255.0, 0xff)
    local abgr = 0xff -- alpha
    abgr = bor(lshift(abgr, 8), b) -- abgr = (abgr << 8) | x
    abgr = bor(lshift(abgr, 8), g) -- abgr = (abgr << 8) | x
    abgr = bor(lshift(abgr, 8), r) -- abgr = (abgr << 8) | x
    abgr = band(abgr, 0xffffffff)
    C.__lu_dd_set_color(abgr)
end

function Debug.drawGrid(axis, pos, size)
    C.__lu_dd_grid(axis, pos, size)
end

function Debug.drawAxis(pos, len, axisHighlight, thickness)
    C.__lu_dd_axis(pos, len, axisHighlight, thickness)
end

function Debug.drawAABB(min, max)
    C.__lu_dd_aabb(min, max)
end

function Debug.drawSphere(center, radius)
    C.__lu_dd_sphere(center, radius)
end

function Debug.finish()
    C.__lu_dd_end()
end

function Debug.gizmoManipulator(entity)
    C.__lu_dd_gizmo_manipulator(entity.id)
end

return Debug
