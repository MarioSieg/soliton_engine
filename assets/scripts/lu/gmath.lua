-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- gmath Module - gmath functions and constants.
--- @module gmath
------------------------------------------------------------------------------

local bit = require 'bit'

local band = bit.band

--- gmath Module
local gmath = {
    e = 2.7182818284590452354, -- e (Euler's number)
    log2e = 1.4426950408889634074, -- log_2 e
    log10e = 0.43429448190325182765, -- log_10 e
    ln2 = 0.69314718055994530942, -- log_e 2
    ln10 = 2.30258509299404568402, -- log_e 10
    tau = 6.28318530717958647692, -- pi * 2
    pi = 3.14159265358979323846, -- pi (circle constant)- Archimedes' constant
    pi_div2 = 1.57079632679489661923, -- pi/2
    pi_div4 = 0.78539816339744830962, -- pi/4
    one_div_pi = 0.31830988618379067154, -- 1/pi
    two_div_pi = 0.63661977236758134308, -- 2/pi
    two_sqrt_pi = 1.12837916709551257390, -- 2/sqrt(pi)
    sqrt2 = 1.41421356237309504880, -- sqrt(2)
    sqrt1_2 = 0.70710678118654752440, -- 1/sqrt(2)
    gamma = 0.57721566490153286060, -- Euler-Mascheroni constant
    infinity = math.huge
}

--- Returns the absolute value of a number.
-- @tparam number x The number
-- @treturn number The absolute value of x
function gmath.abs(x) return math.abs(x) end

--- Returns the arc cosine of a number.
-- @tparam number x The number
-- @treturn number The arc cosine of x
function gmath.acos(x) return math.acos(x) end

--- Returns the arc sine of a number.
-- @tparam number x The number
-- @treturn number The arc sine of x
function gmath.asin(x) return math.asin(x) end

--- Returns the arc tangent of a number.
-- @tparam number y The y coordinate
function gmath.atan(y, x) return math.atan(y, x) end

--- Returns the arc tangent of a number.
-- @tparam number y The y coordinate
function gmath.atan2(y, x) return math.atan2(y, x) end

--- Returns the smallest integer greater than or equal to a number.
-- @tparam number x The number
function gmath.ceil(x) return math.ceil(x) end
function gmath.cos(x) return math.cos(x) end
function gmath.cosh(x) return math.cosh(x) end
function gmath.deg(x) return math.deg() end
function gmath.expEst(x) return math.exp(x) end
function gmath.floor(x) return math.floor(x) end
function gmath.fmod(x, y) return math.fmod(x, y) end
function gmath.frexp(x) return math.frexp(x) end
function gmath.ldexp(m, e) return math.ldexp(m, e) end
function gmath.log(x, base) return math.log(x, base) end
function gmath.max(x, y) return math.max(x, y) end
function gmath.min(x, y) return math.min(x, y) end
function gmath.modf(x) return math.modf(x) end
function gmath.rad(x) return math.rad(x) end
function gmath.random(m, n) return math.random(m, n) end
function gmath.randomSeed(x, y) return math.randomseed(x, y) end
function gmath.sin(x) return math.sin(x) end
function gmath.sinh(x) return math.sinh(x) end
function gmath.sqrt(x) return math.sqrt(x) end
function gmath.tan(x) return math.tan(x) end
function gmath.tanh(x) return math.tanh(x) end
function gmath.tointeger(x) return math.tointeger(x) end
function gmath.type(x) return math.type(x) end
function gmath.ult(m, n) return math.ult(m, n) end

function gmath.clamp(x, lower, upper)
    return gmath.max(lower, gmath.min(upper, x))
end

function gmath.within_interval(x, lower, upper) -- Note: closed interval
    return x >= lower and x <= upper
end

function gmath.is_power_of_2(x)
    return band(x, x - 1) == 0 and x ~= 0
end

function gmath.lerp(x, y, t)
    return (1.0 - t) * x + t * y
end

function gmath.lerp_inv(x, y, v)
    if x ~= y then
        return (v - x ) / (y - x)
    end
    return 0.0
end

function gmath.damp(x, y, lambda, dt)
    return gmath.lerp(x, y, 1.0 - gmath.exp(-lambda * dt))
end

function gmath.smoothstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 2) * (3.0 - 2.0 * x)
end

function gmath.smootherstep(x, min, max)
    if x <= min then return 0.0 end
    if x >= max then return 1.0 end
    x = (x - min) / (max - min)
    return (x ^ 3) * (x * (x * 6.0 - 15.0) + 10.0)
end

function gmath.saturate(x)
    return gmath.max(0.0, gmath.min(1.0, x))
end

function gmath.camera_linearize(depth, znear, zfar)
    return - zfar * znear / (depth * (zfar - znear) - zfar)
end

function gmath.camera_smoothstep(znear, zfar, depth)
    local x = gmath.saturate((depth - znear) / (zfar - znear))
    return (x ^ 2.0) * (3.0 - 2.0 * x)
end

function gmath.loop(t, magnitude)
    return gmath.clamp(t - gmath.floor(t / magnitude) * magnitude, 0.0, magnitude)
end

function gmath.ping_pong(t, magnitude)
    t = gmath.loop(t, magnitude * 2.0)
    return magnitude - gmath.abs(t - magnitude)
end

function gmath.delta_angle(current, target, delay)
    local delta = gmath.loop(target - current, 360.0)
    if delay > 180.0 then delta = delta - 360.0 end
    return delta
end

function gmath.random_interval(lower, greater)
    return lower + gmath.random() * (greater - lower);
end

function gmath.factorial(n)
    if n == 0 then return 1 end
    local r = 1
    for i=2, n do
        r = r * i
    end
    return r
end

function gmath.binomial(n, k)
    return gmath.factorial(n) / gmath.factorial(k) * gmath.factorial(n - k)
end

return gmath
