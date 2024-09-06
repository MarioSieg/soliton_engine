----------------------------------------------------------------------------
-- Lunam Engine Vector2 gmath Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'

local istype = ffi.istype
local rawnew = ffi.typeof('lua_vec2')
local sqrt, cos, sin, atan2, min, max = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max

local zero = rawnew(0.0, 0.0)
local one = rawnew(1.0, 1.0)
local unit_x = rawnew(1.0, 0.0)
local unit_y = rawnew(0.0, 1.0)

local function new(x, y)
    x = x or 0.0
    y = y or x
    return rawnew(x, y)
end

local function from_angle(theta)
    return rawnew(cos(theta), sin(theta))
end

local function magnitude(v)
    local x, y = v.x, v.y
    return sqrt(x*x + y*y)
end

local function sqr_magnitude(v)
    local x, y = v.x, v.y
    return x*x + y*y
end

local function distance(v, other)
    local x, y = other.x - v.x, other.y - v.y
    return sqrt(x*x + y*y)
end

local function sqr_distance(v, other)
    local x, y = other.x - v.x, other.y - v.y
    return x*x + y*y
end

local function dot(v, other)
    local xx, yy = v.x, v.y
    local ox, oy = other.x, other.y
    return xx*ox + yy*oy
end

local function normalize(v)
    return v / magnitude(v)
end

local function reflect(v, normal)
    return v - 2.0*dot(v, normal) * normal
end

local function angle(v)
    return atan2(v.y, v.x)
end

local function rotate(v, theta)
    local c, s = cos(theta), sin(theta)
    return vec2.new(
        c*v.x - s*v.y,
        s*v.x + c*v.y
    )
end

local function clamp(v, lower, upper)
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


ffi.metatype('lua_vec2', {
    __add = function(x, y)
        if istype('lua_vec2', x) then
            if istype('lua_vec2', y) then
                return rawnew(x.x + y.x, x.y + y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x + y, x.y + y)
            end
        elseif istype('lua_vec2', y) then
            if type(x) == 'number' then
                return rawnew(x + y.x, x + y.y)
            end
        end
        error('Invalid operands for vec2 addition.')
    end,
    __sub = function(x, y)
        if istype('lua_vec2', x) then
            if istype('lua_vec2', y) then
                return rawnew(x.x - y.x, x.y - y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x - y, x.y - y)
            end
        elseif istype('lua_vec2', y) then
            if type(x) == 'number' then
                return rawnew(x - y.x, x - y.y)
            end
        end
        error('Invalid operands for vec2 subtraction.')
    end,
    __mul = function(x, y)
        if istype('lua_vec2', x) then
            if istype('lua_vec2', y) then
                return rawnew(x.x * y.x, x.y * y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x * y, x.y * y)
            end
        elseif istype('lua_vec2', y) then
            if type(x) == 'number' then
                return rawnew(x * y.x, x * y.y)
            end
        end
        error('Invalid operands for vec2 multiplication.')
    end,
    __div = function(x, y)
        if istype('lua_vec2', x) then
            if istype('lua_vec2', y) then
                return rawnew(x.x / y.x, x.y / y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x / y, x.y / y)
            end
        elseif istype('lua_vec2', y) then
            if type(x) == 'number' then
                return rawnew(x / y.x, x / y.y)
            end
        end
        error('Invalid operands for vec2 division.')
    end,
    __mod = function(x, y)
        if istype('lua_vec2', x) then
            if istype('lua_vec2', y) then
                return rawnew(x.x % y.x, x.y % y.y)
            elseif type(y) == 'number' then
                return rawnew(x.x % y, x.y % y)
            end
        elseif istype('lua_vec2', y) then
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
        return magnitude(self)
    end,
    __eq = function(x, y)
        local is_vec2 = type(y) == 'cdata' and istype('lua_vec2', y)
        return is_vec2 and x.x == y.x and x.y == y.y
    end,
    __tostring = function(self)
        return string.format('vec2(%f, %f)', self.x, self.y)
    end,
})

local vec2 = setmetatable({
    new = new,
    from_angle = from_angle,
    magnitude = magnitude,
    sqr_magnitude = sqr_magnitude,
    distance = distance,
    sqr_distance = sqr_distance,
    dot = dot,
    normalize = normalize,
    reflect = reflect,
    angle = angle,
    rotate = rotate,
    clamp = clamp,
    unpack = unpack,
    clone = clone,
    zero = zero,
    one = one,
    unit_x = unit_x,
    unit_y = unit_y
}, {
    __call = function(_, x, y)
        return rawnew(x, y)
    end
})

return vec2
