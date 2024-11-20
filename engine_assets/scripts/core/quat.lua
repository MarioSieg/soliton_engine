----------------------------------------------------------------------------
-- Soliton Engine Quaternion gmath Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'

local istype = ffi.istype
local rawnew = ffi.typeof('__vec4')
local sqrt, cos, sin, min, max, acos, abs, atan2 = math.sqrt, math.cos, math.sin, math.min, math.max, math.acos, math.abs, math.atan2

local zero = rawnew(0.0, 0.0, 0.0, 0.0)
local one = rawnew(1.0, 1.0, 1.0, 1.0)
local identity = rawnew(0.0, 0.0, 0.0, 1.0)
local eps = 1e-6
local inv_pi = math.pi * 0.5

local function new(x, y, z, w)
    x = x or 0.0
    y = y or x
    z = z or x
    w = w or 1.0
    return rawnew(x, y, z, w)
end

local function magnitude(q)
    local x, y, z, w = q.x, q.y, q.z, q.w
    return sqrt(x * x + y * y + z * z + w * w)
end

local function sqr_magnitude(q)
    local x, y, z, w = q.x, q.y, q.z, q.w
    return x * x + y * y + z * z + w * w
end

local function normalize(q)
    return q / magnitude(q)
end

local function from_axis_angle(axis, angle)
    local ha = angle * 0.5
    local sa = sin(ha)
    return rawnew(axis.x * sa, axis.y * sa, axis.z * sa, cos(ha))
end

local function from_euler(x, y, z)
    local hx = x * 0.5
    local hy = y * 0.5
    local hz = z * 0.5
    local cr = cos(hx)
    local sr = sin(hx)
    local cp = cos(hy)
    local sp = sin(hy)
    local cy = cos(hz)
    local sy = sin(hz)
    local rw = (cr * cp * cy) + (sr * sp * sy)
    local rx = (sr * cp * cy) - (cr * sp * sy)
    local ry = (cr * sp * cy) + (sr * cp * sy)
    local rz = (cr * cp * sy) - (sr * sp * cy)
    return rawnew(rx, ry, rz, rw)
end

local function to_euler(q)
    local sinr_cosp = 2 * (q.w * q.x + q.y * q.z)
    local cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y)
    local sinp = sqrt(1 + 2 * (q.w * q.y - q.x * q.z))
    local cosp = sqrt(1 - 2 * (q.w * q.y - q.x * q.z))
    local siny_cosp = 2 * (q.w * q.z + q.x * q.y)
    local cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
    local x = atan2(sinr_cosp, cosr_cosp)
    local y = 2 * atan2(sinp, cosp) - inv_pi
    local z = atan2(siny_cosp, cosy_cosp)
    return x, y, z
end

local function from_direction(dir)
    local angle = atan2(dir.z, dir.x) * 0.5
    local qx = 0
    local qy = sin(angle)
    local qz = 0
    local qw = cos(angle)
    return rawnew(qx, qy, qz, qw)
end

local function dot(q, other)
    local x, y, z, w = q.x, q.y, q.z, q.w
    local ox, oy, oz, ow = other.x, other.y, other.z, other.w
    return x * ox + y * oy + z * oz + w * ow
end

local function slerp(x, y, i)
    if x == y then return x end
    local cos_theta = dot(x, y)
    local half_theta = acos(cos_theta)
    local sin_theta = sqrt(1.0 - cos_theta ^ 2.0)
    return x * (sin((1.0 - i) * half_theta) / sin_theta) + y * (sin(i * half_theta) / sin_theta)
end

local function is_norm(q)
    return abs(sqr_magnitude(q) - 1.0) < eps
end

local function angle(q)
    local len = q.x * q.x + q.y * q.y + q.z * q.z
    if len > 0.0 then
        return 2.0 * acos(min(max(q.w, -1.0), 1.0))
    else
        return 0.0
    end
end

local function axis(q)
    local len = q.x*q.x + q.y*q.y + q.z*q.z
    if len > 0.0 then
        local inv = 1.0 / sqrt(len)
        return vec3.new(q.x * inv, q.y * inv, q.z * inv)
    else
        return vec3.unit_x
    end
end

local function conjugate(q)
    return rawnew(-q.x, -q.y, -q.z, q.w)
end

local function invert(q)
    local len = sqr_magnitude(q)
    if len > 0.0 then
        local inv = 1.0 / len
        return rawnew(-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv)
    else
        return zero
    end
end

local function unpack(q)
    return q.x, q.y, q.z, q.w
end

local function clone(q)
    return rawnew(q.x, q.y, q.z, q.w)
end

ffi.metatype('__vec4', {
    __add = function(x, y)
        return rawnew(x.x + y.x, x.y + y.y, x.z + y.z, x.w + y.w)
    end,
    __sub = function(x, y)
        return rawnew(x.x - y.x, x.y - y.y, x.z - y.z, x.w - y.w)
    end,
    __mul = function(x, y)
        if type(y) == 'cdata' and istype('__vec4', y) then
            local xx = x.x * y.w + x.w * y.x + x.y * y.z - x.z * y.y
            local yy = x.y * y.w + x.w * y.y + x.z * y.x - x.x * y.z
            local zz = x.z * y.w + x.w * y.z + x.x * y.y - x.y * y.x
            local ww = x.w * y.w - x.x * y.x - x.y * y.y - x.z * y.z
            return rawnew(xx, yy, zz, ww)
        else
            return rawnew(x.x * y, x.y * y, x.z * y, x.w * y)
        end
    end,
    __div = function(x, y)
        return rawnew(x.x / y, x.y / y, x.z / y, x.w / y)
    end,
    __unm = function(v)
        return rawnew(-v.x, -v.y, -v.z, -v.w)
    end,
    __len = function(self)
        return magnitude(self)
    end,
    __eq = function(x, y)
        local is_vec4 = type(y) == 'cdata' and istype('__vec4', y)
        return is_vec4 and x.x == y.x and x.y == y.y and x.z == y.z and x.w == y.w
    end,
    __tostring = function(self)
        return string.format('quat(%f, %f, %f, %f)', self.x, self.y, self.z, self.w)
    end,
})

local quat = setmetatable({
    new = new,
    normalize = normalize,
    from_axis_angle = from_axis_angle,
    from_euler = from_euler,
    to_euler = to_euler,
    from_direction = from_direction,
    magnitude = magnitude,
    sqr_magnitude = sqr_magnitude,
    dot = dot,
    is_norm = is_norm,
    angle = angle,
    axis = axis,
    conjugate = conjugate,
    invert = invert,
    slerp = slerp,
    unpack = unpack,
    clone = clone,
    zero = zero,
    one = one,
    identity = identity,
}, {
    __call = function(_, x, y, z, w)
        return new(x, y, z, w)
    end,
})

return quat
