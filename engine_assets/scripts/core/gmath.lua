-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- gmath Module - gmath functions and constants.
--- @module gmath
------------------------------------------------------------------------------

local bit = require 'bit'

local band = bit.band
local max, min = math.max, math.min

--- gmath Module
local gmath = {}

function gmath.clamp(x, lower, upper)
    return max(lower, min(upper, x))
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
    for i = 2, n do
        r = r * i
    end
    return r
end

function gmath.binomial(n, k)
    return gmath.factorial(n) / gmath.factorial(k) * gmath.factorial(n - k)
end

return gmath
