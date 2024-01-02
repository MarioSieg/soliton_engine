-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef [[
    typedef struct {
        double x;
        double y;
    } Vec2;
]]

local istype = ffi.istype
local rawnew = ffi.typeof('Vec2')
local sqrt, cos, sin, atan2, min, max = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max

local ZERO = rawnew(0.0, 0.0)
local ONE = rawnew(1.0, 1.0)
local UNIT_X = rawnew(1.0, 0.0)
local UNIT_Y = rawnew(0.0, 1.0)

local function new(x, y)
    assert(type(x) == 'number')
    assert(type(y) == 'number' or y == nil)
    x = x or 0.0
    y = y or x
    return rawnew(x, y)
end

local function fromAngle(theta)
    assert(type(theta) == 'number')
    return rawnew(cos(theta), sin(theta))
end

local function mag(v)
    local x, y = v.x, v.y
    return sqrt(x*x + y*y)
end

local function magSqr(v)
    local x, y = v.x, v.y
    return x*x + y*y
end

local function dist(v, other)
    assert(istype('Vec2', other))
    local x, y = other.x - v.x, other.y - v.y
    return sqrt(x*x + y*y)
end

local function distSqr(v, other)
    assert(istype('Vec2', other))
    local x, y = other.x - v.x, other.y - v.y
    return x*x + y*y
end

local function dot(v, other)
    assert(istype('Vec2', other))
    local xx, yy = v.x, v.y
    local ox, oy = other.x, other.y
    return xx*ox + yy*oy
end

local function norm(v)
    return v / mag(v)
end

local function reflect(v, normal)
    assert(istype('Vec2', normal))
    return v - 2.0*dot(v, normal) * normal
end

local function angle(v)
    return atan2(v.y, v.x)
end

local function rotate(v, theta)
    assert(type(theta) == 'number')
    local c, s = cos(theta), sin(theta)
    return Vec2.new(
        c*v.x - s*v.y,
        s*v.x + c*v.y
    )
end

local function clamp(v, lower, upper)
    assert(istype('Vec2', lower))
    assert(istype('Vec2', upper))
    return new(
        max(lower.x, min(upper.x, v.x)),
        max(lower.y, min(upper.y, v.y))
    )
end

local function unpack(v)
    return v.x, v.y
end

local function clone(v)
    return rawnew(v.x, v.y)
end


ffi.metatype('Vec2', {
    __add = function(x, y)
        if istype('Vec2', x) then
            if istype('Vec2', y) then
                return rawnew(x.x + y.x, x.y + y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x + y, x.y + y)
            end
        elseif istype('Vec2', y) then
            if type(x) == 'number' then
                return rawnew(x + y.x, x + y.y)
            end
        end
        error('Invalid operands for vec2 addition.')
    end,
    __sub = function(x, y)
        if istype('Vec2', x) then
            if istype('Vec2', y) then
                return rawnew(x.x - y.x, x.y - y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x - y, x.y - y)
            end
        elseif istype('Vec2', y) then
            if type(x) == 'number' then
                return rawnew(x - y.x, x - y.y)
            end
        end
        error('Invalid operands for vec2 subtraction.')
    end,
    __mul = function(x, y)
        if istype('Vec2', x) then
            if istype('Vec2', y) then
                return rawnew(x.x * y.x, x.y * y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x * y, x.y * y)
            end
        elseif istype('Vec2', y) then
            if type(x) == 'number' then
                return rawnew(x * y.x, x * y.y)
            end
        end
        error('Invalid operands for vec2 multiplication.')
    end,
    __div = function(x, y)
        if istype('Vec2', x) then
            if istype('Vec2', y) then
                return rawnew(x.x / y.x, x.y / y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x / y, x.y / y)
            end
        elseif istype('Vec2', y) then
            if type(x) == 'number' then
                return rawnew(x / y.x, x / y.y)
            end
        end
        error('Invalid operands for vec2 division.')
    end,
    __mod = function(x, y)
        if istype('Vec2', x) then
            if istype('Vec2', y) then
                return rawnew(x.x % y.x, x.y % y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x % y, x.y % y)
            end
        elseif istype('Vec2', y) then
            if type(x) == 'number' then
                return rawnew(x % y.x, x % y.y)
            end
        end
        error('Invalid operands for vec2 modulo.')
    end,
    __unm = function(v)
        return rawnew(-v.x, -v.y)
    end,
    __len = function(self)
        return mag(self)
    end,
    __eq = function(x, y)
        local is_vec2 = type(y) == 'cdata' and istype('Vec2', y)
        return is_vec2 and x.x == y.x and x.y == y.y
    end,
    __tostring = function(self)
        return string.format('Vec2(%f, %f)', self.x, self.y)
    end,
})

Vec2 = setmetatable({
    new = new,
    fromAngle = fromAngle,
    mag = mag,
    magSqr = magSqr,
    dist = dist,
    distSqr = distSqr,
    dot = dot,
    norm = norm,
    reflect = reflect,
    angle = angle,
    rotate = rotate,
    clamp = clamp,
    unpack = unpack,
    clone = clone,
    ZERO = ZERO,
    ONE = ONE,
    UNIT_X = UNIT_X,
    UNIT_Y = UNIT_Y
}, {
    __call = function(_, x, y)
        return rawnew(x, y)
    end
})
