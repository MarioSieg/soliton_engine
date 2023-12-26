-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'

local band, bor, bxor, lshift, rshift, arshift = bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift

local m = {}
m.E = 2.7182818284590452354 -- e
m.LOG2E = 1.4426950408889634074 -- log_2 e
m.LOG10E = 0.43429448190325182765 -- log_10 e
m.LN2 = 0.69314718055994530942-- log_e 2
m.LN10 = 2.30258509299404568402 -- log_e 10
m.TAU = 6.28318530717958647692 -- pi * 2
m.PI = 3.14159265358979323846 -- pi
m.PI_DIV2 = 1.57079632679489661923 -- pi/2
m.PI_DIV4 = 0.78539816339744830962 -- pi/4
m.ONE_DIV_PI = 0.31830988618379067154 -- 1/pi
m.TWO_DIV_PI = 0.63661977236758134308 -- 2/pi
m.TWO_SQRTPI = 1.12837916709551257390 -- 2/sqrt(pi)
m.SQRT2 = 1.41421356237309504880 -- sqrt(2)
m.SQRT1_2 = 0.70710678118654752440 -- 1/sqrt(2)
m.INFINITY = math.huge

function m.abs(x) return math.abs(x) end
function m.acos(x) return math.acos(x) end
function m.asin(x) return math.asin(x) end
function m.atan(y, x) return math.atan(y, x) end
function m.atan2(y, x) return math.atan2(y, x) end
function m.ceil(x) return math.ceil(x) end
function m.cos(x) return math.cos(x) end
function m.cosh(x) return math.cosh(x) end
function m.deg(x) return math.deg() end
function m.expEst(x) return math.exp(x) end
function m.floor(x) return math.floor(x) end
function m.fmod(x, y) return math.fmod(x, y) end
function m.frexp(x) return math.frexp(x) end
function m.ldexp(m, e) return math.ldexp(m, e) end
function m.log(x, base) return math.log(x, base) end
function m.max(x, y) return math.max(x, y) end
function m.min(x, y) return math.min(x, y) end
function m.modf(x) return math.modf(x) end
function m.rad(x) return math.rad(x) end
function m.random(m, n) return math.random(m, n) end
function m.randomSeed(x, y) return math.randomseed(x, y) end
function m.sin(x) return math.sin(x) end
function m.sinh(x) return math.sinh(x) end
function m.sqrt(x) return math.sqrt(x) end
function m.tan(x) return math.tan(x) end
function m.tanh(x) return math.tanh(x) end
function m.tointeger(x) return math.tointeger(x) end
function m.type(x) return math.type(x) end
function m.ult(m, n) return math.ult(m, n) end

function m.clamp(x, lower, upper)
    return m.max(lower, m.min(upper, x))
end

function m.isPowerOfTwo(x)
    return band(x, x - 1) == 0 and x ~= 0
end

function m.ceilPowerOfTwo(x)
    return 2.0 ^ m.ceil(m.log(x) / m.LN2)
end

function m.floorPowerOfTwo(x)
    return 2.0 ^ m.floor(m.log(x) / m.LN2)
end

function m.lerp(x, y, t)
    return (1.0 - t) * x + t * y
end

function m.lerpInv(x, y, v)
    if x ~= y then
        return (v - x ) / (y - x)
    end
    return 0.0
end

function m.damp(x, y, lambda, dt)
    return m.lerp(x, y, 1.0 - m.exp(-lambda * dt))
end

function m.smoothstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 2) * (3.0 - 2.0 * x)
end

function m.smootherstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 3) * (x * (x * 6.0 - 15.0) + 10.0)
end

function m.saturate(x)
    return m.max(0.0, m.min(1.0, x))
end

function m.cameraLinearize(depth, znear, zfar)
    return - zfar * znear / (depth * (zfar - znear) - zfar)
end

function m.cameraSmoothstep(znear, zfar, depth)
    local x = m.saturate((depth - znear) / (zfar - znear))
    return (x ^ 2.0) * (3.0 - 2.0 * x)
end

function m.loop(t, magnitude)
    return m.clamp(t - m.floor(t / magnitude) * magnitude, 0.0, magnitude)
end

function m.pingPong(t, magnitude)
    t = m.loop(t, magnitude * 2.0)
    return magnitude - m.abs(t - magnitude)
end

function m.deltaAngle(current, target, delay)
    local delta = m.loop(target - current, 360.0)
    if delay > 180.0 then delta = delta - 360.0 end
    return delta
end

function m.randomInterval(lower, greater)
    return lower + m.random() * (greater - lower);
end

return m
