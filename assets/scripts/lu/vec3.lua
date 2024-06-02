----------------------------------------------------------------------------
-- Lunam Engine Vector3 gmath Module
--
-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'

local istype = ffi.istype
local rawnew = ffi.typeof('lua_vec3')
local sqrt, cos, sin, atan2, min, max, random = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max, math.random

local zero = rawnew(0.0, 0.0, 0.0)
local one = rawnew(1.0, 1.0, 1.0)
local unit_x = rawnew(1.0, 0.0, 0.0)
local unit_y = rawnew(0.0, 1.0, 0.0)
local unit_z = rawnew(0.0, 0.0, 1.0)
local left = rawnew(-1.0, 0.0, 0.0)
local right = rawnew(1.0, 0.0, 0.0)
local up = rawnew(0.0, 1.0, 0.0)
local down = rawnew(0.0, -1.0, 0.0)
local forward = rawnew(0.0, 0.0, 1.0)
local backward = rawnew(0.0, 0.0, -1.0)

local function new(x, y, z)
    x = x or 0.0
    y = y or x
    z = z or x
    return rawnew(x, y, z)
end

local function random_scalar_range(min, max)
    return min + random() * (max - min);
end

local function random_range(from, to)
    from = from or 0.0
    to = to or 1.0
    local x = random_scalar_range(from, to)
    local y = random_scalar_range(from, to)
    local z = random_scalar_range(from, to)
    return rawnew(x, y, z)
end

local function random_range_xz(from, to, y)
    from = from or 0.0
    to = to or 1.0
    y = y or 0.0
    local x = random_scalar_range(from, to)
    local z = random_scalar_range(from, to)
    return rawnew(x, y, z)
end

local function from_angles(theta, phi)
    local st, sp, ct, cp = sin(theta), sin(phi), cos(theta), cos(phi)
    return rawnew(st*sp, ct, st*cp)
end

local function magnitude(v)
    local x, y, z = v.x, v.y, v.z
    return sqrt(x*x + y*y + z*z)
end

local function sqr_magnitude(v)
    local x, y, z = v.x, v.y, v.z
    return x*x + y*y + z*z
end

local function distance(v, other)
    local x, y, z = other.x - v.x, other.y - v.y, other.z - v.z
    return sqrt(x*x + y*y + z*z)
end

local function sqr_distance(v, other)
    local x, y, z = other.x - v.x, other.y - v.y, other.z - v.z
    return x*x + y*y + z*z
end

local function dot(v, other)
    local xx, yy, zz = v.x, v.y, v.z
    local ox, oy, oz = other.x, other.y, other.z
    return xx*ox + yy*oy + zz*oz
end

local function cross(v, other)
    local xx, yy, zz = v.x, v.y, v.z
    local ox, oy, oz = other.x, other.y, other.z
    return rawnew(yy*oz - zz*oy, zz*ox - xx*oz, xx*oy - yy*ox)
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
    return vec3.new(
        c*v.x - s*v.y,
        s*v.x + c*v.y,
        v.z
    )
end

local function clamp(v, lower, upper)
    return new(
        max(lower.x, min(upper.x, v.x)),
        max(lower.y, min(upper.y, v.y)),
        max(lower.z, min(upper.z, v.z))
    )
end

local function unpack(v)
    return v.x, v.y, v.z
end

local function tovec2(v)
    return vec2.new(v.x, v.y)
end

local function clone(v)
    return rawnew(v.x, v.y, v.z)
end

local function smooth_damp(current, target, velocity, smooth_t, max_speed, delta_t)
    local out_x, out_y, out_z = 0.0, 0.0, 0.0
    smooth_t = max(0.0001, smooth_t)
    local omega = 2.0 / smooth_t
    local x = omega * delta_t
    local exp = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x)
    local change_x = current.x - target.x
    local change_y = current.y - target.y
    local change_z = current.z - target.z
    local origin = target
    local max_delta = max_speed * smooth_t
    local max_delta_sq = max_delta ^ 2
    local sqr_mag = change_x * change_x + change_y * change_y + change_z * change_z
    if sqr_mag > max_delta_sq then
        local mag = sqrt(sqr_mag)
        change_x = change_x/mag*max_delta
        change_y = change_y/mag*max_delta
        change_z = change_z/mag*max_delta
    end
    target.x = current.x - change_x
    target.y = current.y - change_y
    target.z = current.z - change_z
    local temp_x = (velocity.x + omega*change_x)*delta_t
    local temp_y = (velocity.y + omega*change_y)*delta_t
    local temp_z = (velocity.z + omega*change_z)*delta_t
    velocity.x = (velocity.x - omega*temp_x)*exp
    velocity.y = (velocity.y - omega*temp_y)*exp
    velocity.z = (velocity.z - omega*temp_z)*exp
    out_x = target.x + (change_x + temp_x)*exp
    out_y = target.y + (change_y + temp_y)*exp
    out_z = target.z + (change_z + temp_z)*exp
    local origMinusCurrent_x = origin.x - current.x
    local origMinusCurrent_y = origin.y - current.y
    local origMinusCurrent_z = origin.z - current.z
    local outMinusOrig_x = out_x - origin.x
    local outMinusOrig_y = out_y - origin.y
    local outMinusOrig_z = out_z - origin.z
    if origMinusCurrent_x * outMinusOrig_x + origMinusCurrent_y * outMinusOrig_y + origMinusCurrent_z * outMinusOrig_z > 0 then
        out_x = origin.x
        out_y = origin.y
        out_z = origin.z
        velocity.x = (out_x - origin.x)/delta_t
        velocity.y = (out_y - origin.y)/delta_t
        velocity.z = (out_z - origin.z)/delta_t
    end
    return rawnew(out_x, out_y, out_z), velocity
end

ffi.metatype('lua_vec3', {
    __add = function(x, y)
        if istype('lua_vec3', x) then
            if istype('lua_vec3', y) then
                return new(x.x + y.x, x.y + y.y, x.z + y.z)
            elseif type(y) == 'number' then
                return new(x.x + y, x.y + y, x.z + y)
            end
        elseif istype('lua_vec3', y) then
            if type(x) == 'number' then
                return new(x + y.x, x + y.y, x + y.z)
            end
        end
        error('Invalid operands for vec3 addition.')
    end,
    __sub = function(x, y)
        if istype('lua_vec3', x) then
            if istype('lua_vec3', y) then
                return new(x.x - y.x, x.y - y.y, x.z - y.z)
            elseif type(y) == 'number' then
                return new(x.x - y, x.y - y, x.z - y)
            end
        elseif istype('lua_vec3', y) then
            if type(x) == 'number' then
                return new(x - y.x, x - y.y, x - y.z)
            end
        end
        error('Invalid operands for vec3 subtraction.')
    end,
    __mul = function(x, y)
        if istype('lua_vec3', x) then
            if istype('lua_vec3', y) then
                return new(x.x * y.x, x.y * y.y, x.z * y.z)
            elseif istype('lua_vec4', y) then -- quaternion rotation
                local ix = y.w*x.x + y.y*x.z - y.z*x.y
                local iy = y.w*x.y + y.z*x.x - y.x*x.z
                local iz = y.w*x.z + y.x*x.y - y.y*x.x
                local iw = -y.x*x.x - y.y*x.y - y.z*x.z
                local xx = ix*y.w + iw*-y.x + iy*-y.z - iz*-y.y
                local yy = iy*y.w + iw*-y.y + iz*-y.x - ix*-y.z
                local zz = iz*y.w + iw*-y.z + ix*-y.y - iy*-y.x
                return rawnew(xx, yy, zz)
            elseif type(y) == 'number' then
                return new(x.x * y, x.y * y, x.z * y)
            end
        elseif istype('lua_vec3', y) then
            if type(x) == 'number' then
                return new(x * y.x, x * y.y, x * y.z)
            end
        end
        error('Invalid operands for vec3 multiplication.')
    end,
    __div = function(x, y)
        if istype('lua_vec3', x) then
            if istype('lua_vec3', y) then
                return new(x.x / y.x, x.y / y.y, x.z / y.z)
            elseif type(y) == 'number' then
                return new(x.x / y, x.y / y, x.z / y)
            end
        elseif istype('lua_vec3', y) then
            if type(x) == 'number' then
                return new(x / y.x, x / y.y, x / y.z)
            end
        end
        error('Invalid operands for vec3 division.')
    end,
    __mod = function(x, y)
        if istype('lua_vec3', x) then
            if istype('lua_vec3', y) then
                return new(x.x % y.x, x.y % y.y, x.z % y.z)
            elseif type(y) == 'number' then
                return new(x.x % y, x.y % y, x.z % y)
            end
        elseif istype('lua_vec3', y) then
            if type(x) == 'number' then
                return new(x % y.x, x % y.y, x % y.z)
            end
        end
        error('Invalid operands for vec3 modulo.')
    end,
    __unm = function(v)
        return new(-v.x, -v.y, -v.z)
    end,
    __len = function(self)
        return magnitude(self)
    end,
    __eq = function(x, y)
        local is_vec3 = type(y) == 'cdata' and istype('lua_vec3', y)
        return is_vec3 and x.x == y.x and x.y == y.y and x.z == y.z
    end,
    __tostring = function(self)
        return string.format('vec3(%.3f, %.3f, %.3f)', self.x, self.y, self.z)
    end,
})

local vec3 = setmetatable({
    new = new,
    random_range = random_range,
    random_range_xz = random_range_xz,
    from_angles = from_angles,
    magnitude = magnitude,
    sqr_magnitude = sqr_magnitude,
    distance = distance,
    sqr_distance = sqr_distance,
    dot = dot,
    cross = cross,
    normalize = normalize,
    reflect = reflect,
    angle = angle,
    rotate = rotate,
    clamp = clamp,
    unpack = unpack,
    tovec2 = tovec2,
    clone = clone,
    smooth_damp = smooth_damp,
    zero = zero,
    one = one,
    unit_x = unit_x,
    unit_y = unit_y,
    unit_z = unit_z,
    left = left,
    right = right,
    up = up,
    down = down,
    forward = forward,
    backward = backward,
}, {
    __call = function(_, x, y, z)
        return new(x, y, z)
    end
})

return vec3
