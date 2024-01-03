-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bor, band, lshift = bit.bor, bit.band, bit.lshift

ffi.cdef[[
    typedef struct {
        double x;
        double y;
        double z;
    } Vec3;
    void __lu_dd_begin(void);
    void __lu_dd_set_wireframe(bool wireframe);
    void __lu_dd_set_color(uint32_t abgr);
    void __lu_dd_grid(int axis, Vec3 pos, uint32_t size, float step);
    void __lu_dd_axis(Vec3 pos, float len, int axis_highlight, float thickness);
    void __lu_dd_aabb(Vec3 min, Vec3 max);
    void __lu_dd_sphere(Vec3 center, float radius);
    void __lu_dd_end(void);
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
    assert(type(isWireframe) == 'boolean')
    C.__lu_dd_set_wireframe(isWireframe)
end

function Debug.setColor(rgb)
    assert(istype('Vec3', rgb))
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

function Debug.drawGrid(axis, pos, size, step)
    assert(type(axis) == 'number')
    assert(istype('Vec3', pos))
    assert(type(size) == 'number')
    assert(type(step) == 'number')
    C.__lu_dd_grid(axis, pos, size, step)
end

function Debug.drawAxis(pos, len, axis_highlight, thickness)
    assert(istype('Vec3', pos))
    assert(type(len) == 'number')
    assert(type(axis_highlight) == 'number')
    assert(type(thickness) == 'number')
    C.__lu_dd_axis(pos, len, axis_highlight, thickness)
end

function Debug.drawAABB(min, max)
    assert(istype('Vec3', min))
    assert(istype('Vec3', max))
    C.__lu_dd_aabb(min, max)
end

function Debug.drawSphere(center, radius)
    assert(istype('Vec3', center))
    assert(type(radius) == 'number')
    C.__lu_dd_sphere(center, radius)
end

function Debug.finish()
    C.__lu_dd_end()
end

return Debug
