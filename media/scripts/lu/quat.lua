-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local istype = ffi.istype
local rawnew = ffi.typeof('lua_vec4')
local sqrt, cos, sin, atan2, min, max, acos, abs = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max, math.acos, math.abs

local ZERO = rawnew(0.0, 0.0, 0.0, 0.0)
local ONE = rawnew(1.0, 1.0, 1.0, 1.0)
local IDENTITY = rawnew(0.0, 0.0, 0.0, 1.0)
local EPSILON = 1e-6

local function new(x, y, z, w)
    assert(type(x) == 'number')
    assert(type(y) == 'number' or y == nil)
    assert(type(z) == 'number' or z == nil)
    assert(type(w) == 'number' or w == nil)
    x = x or 0.0
    y = y or x
    z = z or x
    w = w or 1.0
    return rawnew(x, y, z, w)
end

local function mag(q)
    local x, y, z, w = q.x, q.y, q.z, q.w
    return sqrt(x*x + y*y + z*z + w*w)
end

local function magSqr(q)
    local x, y, z, w = q.x, q.y, q.z, q.w
    return x*x + y*y + z*z + w*w
end

local function norm(q)
    return q / mag(q)
end

local function fromAxisAngle(vec)
    assert(istype('lua_vec3', vec))
    local angle = #vec
    if angle == 0.0 then
        return IDENTITY
    end
    local axis = vec / angle
    local s, c = sin(angle * 0.5), cos(angle * 0.5)
    return norm(new(axis.x * s, axis.y * s, axis.z * s, c))
end

local function fromYawPitchRoll(yaw, pitch, roll)
    assert(type(pitch) == 'number')
    assert(type(yaw) == 'number')
    assert(type(roll) == 'number')
    local hp = pitch * 0.5
    local cp = cos(hp)
    local sp = sin(hp)
    local hy = yaw * 0.5
    local cy = cos(hy)
    local sy = sin(hy)
    local hr = roll * 0.5
    local cr = cos(hr)
    local sr = sin(hr)
    local x = cr*sp*cy + sr*cp*sy
    local y = cr*cp*sy - sr*sp*cy
    local z = sr*cp*cy - cr*sp*sy
    local w = cr*cp*cy + sr*sp*sy
    return rawnew(x, y, z, w)
end

local function dot(q, other)
    assert(istype('lua_vec4', other))
    local x, y, z, w = q.x, q.y, q.z, q.w
    local ox, oy, oz, ow = other.x, other.y, other.z, other.w
    return x*ox + y*oy + z*oz + w*ow
end

local function slerp(x, y, i)
    assert(istype('lua_vec4', x))
    assert(istype('lua_vec4', y))
    assert(type(i) == 'number')
    if x == y then return x end
    local cosHalfTheta = dot(x, y)
    local halfTheta = acos(cosHalfTheta)
    local sinHalfTheta = sqrt(1.0 - cosHalfTheta ^ 2.0)
    return x * (sin((1.0 - i) * halfTheta) / sinHalfTheta) + y * (sin(i * halfTheta) / sinHalfTheta)
end

local function is_norm(q)
    return abs(magSqr(q) - 1.0) < EPSILON
end

local function angle(q)
    local len = q.x*q.x + q.y*q.y + q.z*q.z
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
        return Vec3.new(q.x * inv, q.y * inv, q.z * inv)
    else
        return Vec3.UNIT_X
    end
end

local function conjugate(q)
    return rawnew(-q.x, -q.y, -q.z, q.w)
end

local function invert(q)
    local len = magSqr(q)
    if len > 0.0 then
        local inv = 1.0 / len
        return rawnew(-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv)
    else
        return ZERO
    end
end

local function unpack(q)
    return q.x, q.y, q.z, q.w
end

local function clone(q)
    return rawnew(q.x, q.y, q.z, q.w)
end

ffi.metatype('lua_vec4', {
    _add = function(x, y)
        assert(istype('lua_vec4', y))
        return rawnew(x.x + y.x, x.y + y.y, x.z + y.z, x.w + y.w)
    end,
    _sub = function(x, y)
        assert(istype('lua_vec4', y))
        return rawnew(x.x - y.x, x.y - y.y, x.z - y.z, x.w - y.w)
    end,
    _mul = function(x, y)
        if type(y) == 'cdata' and istype('lua_vec4', y) then
            local x = x.x*y.w + x.w*y.x + x.y*y.z - x.z*y.y
            local y = x.y*y.w + x.w*y.y + x.z*y.x - x.x*y.z
            local z = x.z*y.w + x.w*y.z + x.x*y.y - x.y*y.x
            local w = x.w*y.w - x.x*y.x - x.y*y.y - x.z*y.z
            return rawnew(x, y, z, w)
        else
            return rawnew(x.x*y, x.y*y, x.z*y, x.w*y)
        end
    end,
    __unm = function(v)
        return rawnew(-v.x, -v.y, -v.z, -v.w)
    end,
    __len = function(self)
        return mag(self)
    end,
    __eq = function(x, y)
        local is_vec3 = type(y) == 'cdata' and istype('lua_vec4', y)
        return is_vec3 and x.x == y.x and x.y == y.y and x.z == y.z and x.w == y.w
    end,
    __tostring = function(self)
        return string.format('Quat(%f, %f, %f, %f)', self.x, self.y, self.z, self.w)
    end,
})

local Quat = setmetatable({
    new = new,
    norm = norm,
    fromAxisAngle = fromAxisAngle,
    fromYawPitchRoll = fromYawPitchRoll,
    mag = mag,
    magSqr = magSqr,
    dot = dot,
    is_norm = is_norm,
    angle = angle,
    axis = axis,
    conjugate = conjugate,
    invert = invert,
    slerp = slerp,
    unpack = unpack,
    clone = clone,
    ZERO = ZERO,
    ONE = ONE,
    IDENTITY = IDENTITY,
}, {
    __call = function(_, x, y, z, w)
        return new(x, y, z, w)
    end,
})

return Quat
