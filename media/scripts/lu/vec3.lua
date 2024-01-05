-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local istype = ffi.istype
local rawnew = ffi.typeof('lua_vec3')
local sqrt, cos, sin, atan2, min, max = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max

local ZERO = rawnew(0.0, 0.0, 0.0)
local ONE = rawnew(1.0, 1.0, 1.0)
local UNIT_X = rawnew(1.0, 0.0, 0.0)
local UNIT_Y = rawnew(0.0, 1.0, 0.0)
local UNIT_Z = rawnew(0.0, 0.0, 1.0)
local LEFT = rawnew(-1.0, 0.0, 0.0)
local RIGHT = rawnew(1.0, 0.0, 0.0)
local UP = rawnew(0.0, 1.0, 0.0)
local DOWN = rawnew(0.0, -1.0, 0.0)
local FORWARD = rawnew(0.0, 0.0, 1.0)
local BACKWARD = rawnew(0.0, 0.0, -1.0)

local function new(x, y, z)
    assert(type(x) == 'number')
    assert(type(y) == 'number' or y == nil)
    assert(type(z) == 'number' or z == nil)
    x = x or 0.0
    y = y or x
    z = z or x
    return rawnew(x, y, z)
end

local function fromAngles(theta, phi)
    assert(type(theta) == 'number')
    assert(type(phi) == 'number')
    local st, sp, ct, cp = sin(theta), sin(phi), cos(theta), cos(phi)
    return rawnew(st*sp, ct, st*cp)
end

local function mag(v)
    local x, y, z = v.x, v.y, v.z
    return sqrt(x*x + y*y + z*z)
end

local function magSqr(v)
    local x, y, z = v.x, v.y, v.z
    return x*x + y*y + z*z
end

local function dist(v, other)
    assert(istype('lua_vec3', other))
    local x, y, z = other.x - v.x, other.y - v.y, other.z - v.z
    return sqrt(x*x + y*y + z*z)
end

local function distSqr(v, other)
    assert(istype('lua_vec3', other))
    local x, y, z = other.x - v.x, other.y - v.y, other.z - v.z
    return x*x + y*y + z*z
end

local function dot(v, other)
    assert(istype('lua_vec3', other))
    local xx, yy, zz = v.x, v.y, v.z
    local ox, oy, oz = other.x, other.y, other.z
    return xx*ox + yy*oy + zz*oz
end

local function cross(v, other)
    assert(istype('lua_vec3', other))
    local xx, yy, zz = v.x, v.y, v.z
    local ox, oy, oz = other.x, other.y, other.z
    return rawnew(yy*oz - zz*oy, zz*ox - xx*oz, xx*oy - yy*ox)
end

local function norm(v)
    return v / mag(v)
end

local function reflect(v, normal)
    assert(istype('lua_vec3', normal))
    return v - 2.0*dot(v, normal) * normal
end

local function angle(v)
    return atan2(v.y, v.x)
end

local function rotate(v, theta)
    assert(type(theta) == 'number')
    local c, s = cos(theta), sin(theta)
    return Vec3.new(
        c*v.x - s*v.y,
        s*v.x + c*v.y,
        v.z
    )
end

local function clamp(v, lower, upper)
    assert(istype('lua_vec3', lower))
    assert(istype('lua_vec3', upper))
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
    return Vec2.new(v.x, v.y)
end

local function clone(v)
    return rawnew(v.x, v.y, v.z)
end

local function smoothDamp(current, target, velocity, smoothTime, maxSpeed, deltaTime)
    assert(istype('lua_vec3', current))
    assert(istype('lua_vec3', target))
    assert(istype('lua_vec3', velocity))
    assert(type(smoothTime) == 'number')
    assert(type(maxSpeed) == 'number')
    assert(type(deltaTime) == 'number')

    local out_x, out_y, out_z = 0.0, 0.0, 0.0
    smoothTime = max(0.0001, smoothTime)
    local omega = 2.0 / smoothTime
    local x = omega * deltaTime
    local exp = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x)

    local change_x = current.x - target.x
    local change_y = current.y - target.y
    local change_z = current.z - target.z

    local origin = target
    local maxChange = maxSpeed * smoothTime
    local maxChangeSq = maxChange ^ 2
    local sqrMag = change_x * change_x + change_y * change_y + change_z * change_z

    if sqrMag > maxChangeSq then
        local mag = sqrt(sqrMag)
        change_x = change_x / mag * maxChange
        change_y = change_y / mag * maxChange
        change_z = change_z / mag * maxChange
    end

    target.x = current.x - change_x
    target.y = current.y - change_y
    target.z = current.z - change_z

    local temp_x = (velocity.x + omega * change_x) * deltaTime
    local temp_y = (velocity.y + omega * change_y) * deltaTime
    local temp_z = (velocity.z + omega * change_z) * deltaTime

    velocity.x = (velocity.x - omega * temp_x) * exp
    velocity.y = (velocity.y - omega * temp_y) * exp
    velocity.z = (velocity.z - omega * temp_z) * exp

    out_x = target.x + (change_x + temp_x) * exp
    out_y = target.y + (change_y + temp_y) * exp
    out_z = target.z + (change_z + temp_z) * exp

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

        velocity.x = (out_x - origin.x) / deltaTime
        velocity.y = (out_y - origin.y) / deltaTime
        velocity.z = (out_z - origin.z) / deltaTime
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
                local ix =  y.w*y.x + y.y*x.z - y.z*x.y
                local iy =  y.w*y.y + y.z*x.x - y.x*x.z
                local iz =  y.w*y.z + y.x*x.y - y.y*x.x
                local iw = -y.x*x.x - y.y*x.y - y.z*x.z
                local x = ix*y.w + iw*-y.x + iy*-y.z - iz*-y.y
                local yy = iy*y.w + iw*-y.y + iz*-y.x - ix*-y.z
                local z = iz*y.w + iw*-y.z + ix*-y.y - iy*-y.x
                return rawnew(x, yy, z)
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
        return mag(self)
    end,
    __eq = function(x, y)
        local is_vec3 = type(y) == 'cdata' and istype('lua_vec3', y)
        return is_vec3 and x.x == y.x and x.y == y.y and x.z == y.z
    end,
    __tostring = function(self)
        return string.format('Vec3(%.3f, %.3f, %.3f)', self.x, self.y, self.z)
    end,
})

local Vec3 = setmetatable({
    new = new,
    fromAngles = fromAngles,
    mag = mag,
    magSqr = magSqr,
    dist = dist,
    distSqr = distSqr,
    dot = dot,
    cross = cross,
    norm = norm,
    reflect = reflect,
    angle = angle,
    rotate = rotate,
    clamp = clamp,
    unpack = unpack,
    tovec2 = tovec2,
    clone = clone,
    smoothDamp = smoothDamp,
    ZERO = ZERO,
    ONE = ONE,
    UNIT_X = UNIT_X,
    UNIT_Y = UNIT_Y,
    UNIT_Z = UNIT_Z,
    LEFT = LEFT,
    RIGHT = RIGHT,
    UP = UP,
    DOWN = DOWN,
    FORWARD = FORWARD,
    BACKWARD = BACKWARD,
}, {
    __call = function(_, x, y, z)
        return new(x, y, z)
    end
})

return Vec3
