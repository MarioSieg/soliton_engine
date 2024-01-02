-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'

local band, bor, bxor, lshift, rshift, arshift = bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift

local Math = {
    E = 2.7182818284590452354, -- e
    LOG2E = 1.4426950408889634074, -- log_2 e
    LOG10E = 0.43429448190325182765, -- log_10 e
    LN2 = 0.69314718055994530942, -- log_e 2
    LN10 = 2.30258509299404568402, -- log_e 10
    TAU = 6.28318530717958647692, -- pi * 2
    PI = 3.14159265358979323846, -- pi
    PI_DIV2 = 1.57079632679489661923, -- pi/2
    PI_DIV4 = 0.78539816339744830962, -- pi/4
    ONE_DIV_PI = 0.31830988618379067154, -- 1/pi
    TWO_DIV_PI = 0.63661977236758134308, -- 2/pi
    TWO_SQRTPI = 1.12837916709551257390, -- 2/sqrt(pi)
    SQRT2 = 1.41421356237309504880, -- sqrt(2)
    SQRT1_2 = 0.70710678118654752440, -- 1/sqrt(2)
    INFINITY = math.huge
}

function Math.abs(x) return math.abs(x) end
function Math.acos(x) return math.acos(x) end
function Math.asin(x) return math.asin(x) end
function Math.atan(y, x) return math.atan(y, x) end
function Math.atan2(y, x) return math.atan2(y, x) end
function Math.ceil(x) return math.ceil(x) end
function Math.cos(x) return math.cos(x) end
function Math.cosh(x) return math.cosh(x) end
function Math.deg(x) return math.deg() end
function Math.expEst(x) return math.exp(x) end
function Math.floor(x) return math.floor(x) end
function Math.fmod(x, y) return math.fmod(x, y) end
function Math.frexp(x) return math.frexp(x) end
function Math.ldexp(m, e) return math.ldexp(m, e) end
function Math.log(x, base) return math.log(x, base) end
function Math.max(x, y) return math.max(x, y) end
function Math.min(x, y) return math.min(x, y) end
function Math.modf(x) return math.modf(x) end
function Math.rad(x) return math.rad(x) end
function Math.random(m, n) return math.random(m, n) end
function Math.randomSeed(x, y) return math.randomseed(x, y) end
function Math.sin(x) return math.sin(x) end
function Math.sinh(x) return math.sinh(x) end
function Math.sqrt(x) return math.sqrt(x) end
function Math.tan(x) return math.tan(x) end
function Math.tanh(x) return math.tanh(x) end
function Math.tointeger(x) return math.tointeger(x) end
function Math.type(x) return math.type(x) end
function Math.ult(m, n) return math.ult(m, n) end

function Math.clamp(x, lower, upper)
    return Math.max(lower, Math.min(upper, x))
end

function Math.isPowerOfTwo(x)
    return band(x, x - 1) == 0 and x ~= 0
end

function Math.ceilPowerOfTwo(x)
    return 2.0 ^ Math.ceil(Math.log(x) / Math.LN2)
end

function Math.floorPowerOfTwo(x)
    return 2.0 ^ Math.floor(Math.log(x) / Math.LN2)
end

function Math.lerp(x, y, t)
    return (1.0 - t) * x + t * y
end

function Math.lerpInv(x, y, v)
    if x ~= y then
        return (v - x ) / (y - x)
    end
    return 0.0
end

function Math.damp(x, y, lambda, dt)
    return Math.lerp(x, y, 1.0 - Math.exp(-lambda * dt))
end

function Math.smoothstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 2) * (3.0 - 2.0 * x)
end

function Math.smootherstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 3) * (x * (x * 6.0 - 15.0) + 10.0)
end

function Math.saturate(x)
    return Math.max(0.0, Math.min(1.0, x))
end

function Math.cameraLinearize(depth, znear, zfar)
    return - zfar * znear / (depth * (zfar - znear) - zfar)
end

function Math.cameraSmoothstep(znear, zfar, depth)
    local x = Math.saturate((depth - znear) / (zfar - znear))
    return (x ^ 2.0) * (3.0 - 2.0 * x)
end

function Math.loop(t, magnitude)
    return Math.clamp(t - Math.floor(t / magnitude) * magnitude, 0.0, magnitude)
end

function Math.pingPong(t, magnitude)
    t = Math.loop(t, magnitude * 2.0)
    return magnitude - Math.abs(t - magnitude)
end

function Math.deltaAngle(current, target, delay)
    local delta = Math.loop(target - current, 360.0)
    if delay > 180.0 then delta = delta - 360.0 end
    return delta
end

function Math.randomInterval(lower, greater)
    return lower + Math.random() * (greater - lower);
end

return Math
