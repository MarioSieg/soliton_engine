-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local FFI = require 'ffi'

local pow, sin, cos, pi, sqrt, abs, asin = math.pow, math.sin, math.cos, math.pi, math.sqrt, math.abs, math.asin
local band, bor, bxor, lshift, rshift, arshift = bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift

local M = {}

M.E = 2.7182818284590452354 -- e
M.LOG2E = 1.4426950408889634074 -- log_2 e
M.LOG10E = 0.43429448190325182765 -- log_10 e
M.LN2 = 0.69314718055994530942-- log_e 2
M.LN10 = 2.30258509299404568402 -- log_e 10
M.TAU = 6.28318530717958647692 -- pi * 2
M.PI = 3.14159265358979323846 -- pi
M.PI_DIV2 = 1.57079632679489661923 -- pi/2
M.PI_DIV4 = 0.78539816339744830962 -- pi/4
M.ONE_DIV_PI = 0.31830988618379067154 -- 1/pi
M.TWO_DIV_PI = 0.63661977236758134308 -- 2/pi
M.TWO_SQRTPI = 1.12837916709551257390 -- 2/sqrt(pi)
M.SQRT2 = 1.41421356237309504880 -- sqrt(2)
M.SQRT1_2 = 0.70710678118654752440 -- 1/sqrt(2)
M.INFINITY = math.huge

function M.abs(x) return math.abs(x) end
function M.acos(x) return math.acos(x) end
function M.asin(x) return math.asin(x) end
function M.atan(y, x) return math.atan(y, x) end
function M.atan2(y, x) return math.atan2(y, x) end
function M.ceil(x) return math.ceil(x) end
function M.cos(x) return math.cos(x) end
function M.cosh(x) return math.cosh(x) end
function M.deg(x) return math.deg() end
function M.expEst(x) return math.exp(x) end
function M.floor(x) return math.floor(x) end
function M.fmod(x, y) return math.fmod(x, y) end
function M.frexp(x) return math.frexp(x) end
function M.ldexp(m, e) return math.ldexp(m, e) end
function M.log(x, base) return math.log(x, base) end
function M.max(x, y) return math.max(x, y) end
function M.min(x, y) return math.min(x, y) end
function M.modf(x) return math.modf(x) end
function M.pow(x, y) return x ^ y end
function M.rad(x) return math.rad(x) end
function M.random(m, n) return math.random(m, n) end
function M.randomSeed(x, y) return math.randomseed(x, y) end
function M.sin(x) return math.sin(x) end
function M.sinh(x) return math.sinh(x) end
function M.sqrt(x) return math.sqrt(x) end
function M.tan(x) return math.tan(x) end
function M.tanh(x) return math.tanh(x) end
function M.tointeger(x) return math.tointeger(x) end
function M.type(x) return math.type(x) end
function M.ult(m, n) return math.ult(m, n) end

function M.clamp(x, lower, upper)
    return M.max(lower, M.min(upper, x))
end

function M.isPowerOfTwo(x)
    return band(x, x - 1) == 0 and x ~= 0
end

function M.ceilPowerOfTwo(x)
return M.pow(2, M.ceil(M.log(x) / M.LN2))
end

function M.floorPowerOfTwo(x)
    return M.pow(2, M.floor(M.log(x) / M.LN2))
end

function M.lerp(x, y, t)
    return (1 - t) * x + t * y
end

local POW_LUT = { -- 1+2^-i
    1.5, 1.25, 1.125, 1.0625,
    1.03125, 1.015625, 1.0078125,
    1.00390625, 1.001953125, 1.0009765625,
    1.00048828125, 1.000244140625, 1.0001220703125,
    1.00006103515625, 1.000030517578125
}

local LOG_LUT = { -- log(1+2^-i)
    0.40546510810816438486, 0.22314355131420976486, 0.11778303565638345574,
    0.06062462181643483994, 0.03077165866675368733, 0.01550418653596525448,
    0.00778214044205494896, 0.00389864041565732289, 0.00195122013126174934,
    0.00097608597305545892, 0.00048816207950135119, 0.00024411082752736271,
    0.00012206286252567737, 0.00006103329368063853, 0.00003051711247318638
}

function M.intPow(x, n)
    if n == 0 then
        return 1
    elseif n < 0 then
        x = 1 / x
        n = -n
    end
    local y = 1
    while n > 1 do
        if n % 2 == 0 then
            n = n / 2
        else
            y = x * y
            n = (n - 1) / 2
        end
        x = x * x
    end
    return x * y
end

function M.expEst(x)
    local xint, xfract = M.modf(x)
    local exint = M.intPow(M.E, xint)
    local exfract = 1 + xfract + (xfract * xfract / 2) + (xfract * xfract * xfract / 6) + (xfract * xfract * xfract * xfract / 24)
    return exint * exfract
end

function M.logEst(x)
    local xmant, xexp = M.frexp(x)
    if xmant == 0.5 then
        return M.LN2 * (xexp-1)
    end
    local arg = xmant * 2
    local prod = 1
    local sum = 0
    for i = 1, 15 do
        local prod2 = prod * POW_LUT[i]
        if prod2 < arg then
            prod = prod2
            sum = sum + LOG_LUT[i]
        end
    end
    return sum + M.LN2 * (xexp - 1)
end
	
function M.powEst(x, y)
    local yint, yfract = M.modf(y)
    local xyint = M.intPow(x, yint)
    local xyfract = M.expEst(M.logEst(x) * yfract)
    return xyint * xyfract -- x ^ (yint + yfract)
end

function M.sinEst(x)
    local over = M.floor(x / (M.TAU / 2)) % 2 == 0 
    x = M.TAU / 4 - x % (M.TAU / 2)
    local abs = 1 - (20 * x * x) / (4 * x * x + M.TAU * M.TAU)
    return over and abs or -abs
end

function M.cosEst(x)
    local over = M.floor((M.TAU / 4 - x) / (M.TAU / 2)) % 2 == 0
    x = M.TAU / 4 - (M.TAU / 4 - x) % (M.TAU/  2)
    local abs = 1 - (20 * x * x) / (4 * x * x + M.TAU * M.TAU)
    return over and abs or -abs
end

function M.tanEst(x)
    return M.sinEst(x) / M.cosEst(x)
end

function M.asinEst(x)
    local positiveX, x = x > 0, M.abs(x)
    local resultForAbsoluteX = M.TAU / 4 - M.sqrt(M.TAU / M.TAU * (1 - x)) / (2 * M.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX
end

function M.acosEst(x)
    local positiveX, x = x > 0, M.abs(x)
    local resultForAbsoluteX = M.sqrt(M.TAU * M.TAU * (1 - x)) / (2 * M.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX + M.TAU / 2
end

function M.atanEst(x)
    x = x / M.sqrt(1 + x*x)
    local positiveX, x = x > 0, M.abs(x)
    local resultForAbsoluteX = M.TAU / 4 - M.sqrt(M.TAU * M.TAU * (1 - x)) / (2 * M.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX
end

function M.atan2Est(y, x)
    if x == 0 and y == 0 then
        return 0
    end
    local theta = M.atan(y/x)
    theta = x == 0 and M.TAU / 4 * y / M.abs(y) or x < 0 and theta + M.TAU / 2 or theta
    theta = theta > M.TAU / 2 and theta - M.TAU or theta
    return theta
end

function M.sinhEst(x)
    local ex = M.expEst(x)
    return (ex - 1 / ex) / 2
end

function M.coshEst(x)
    local ex = M.expEst(x)
    return (ex + 1 / ex) / 2
end

function M.tanhEst(x)
    local ex = M.expEst(x)
    return (ex - 1 / ex) / (ex + 1 / ex)
end

-- https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/
function M.inverseLerp(x, y, value)
    if x ~= y then
        return (value - x ) / (y - x)
    end
    return 0
end

-- http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
function M.damp(x, y, lambda, dt)
    return M.lerp(x, y, 1 - M.expEst(-lambda * dt))
end

-- http://en.wikipedia.org/wiki/Smoothstep
function M.smoothstep(x, min, max)
    if x <= min then return 0 end
    if x >= max then return 1 end
    x = (x - min) / (max - min)
    return x * x * (3 - 2 * x)
end

-- http://en.wikipedia.org/wiki/Smoothstep
function M.smootherstep(x, min, max)
    if x <= min then return 0 end
    if x >= max then return 1 end
    x = (x - min) / (max - min)
    return x * x * x * (x * (x * 6 - 15) + 10)
end

function M.saturate(x)
    return M.max(0, M.min(1, x))
end

function M.cameraLinearize(depth, znear, zfar)
    return - zfar * znear / (depth * (zfar - znear) - zfar)
end

function M.cameraSmoothstep(znear, zfar, depth)
    local x = M.saturate((depth - znear) / (zfar - znear))
    return (x ^ 2) * (3 - 2 * x)
end

local seed = 0x12D687 -- Deterministic pseudo-random float in the interval [ 0, 1 ] (Mulberry32 generator)

function M.seededRandom(s)
    seed = seed or s
    seed = seed + 0x6D2B79F5
    local t = seed
    t = bxor(t, arshift(t, 15)) * bor(t, 1)
    t = bxor(t, t + arshift(bxor(t, t), 7) * bor(t, 61))
    return arshift(bxor(t, t), 14) / 4294967296
end

function M.loop(t, magnitude)
    return M.clamp(t - M.floor(t / magnitude) * magnitude, 0.0, magnitude)
end

function M.pingPong(t, magnitude)
    t = M.loop(t, magnitude * 2.0)
    return magnitude - M.abs(t - magnitude)
end

function M.deltaAngle(current, target, delay)
    local delta = M.loop(target - current, 360.0)
    if delay > 180.0 then delta = delta - 360.0 end
    return delta
end

function M.randomRange(lower, greater)
    return lower + M.random() * (greater - lower);
end

function M.linear(t, b, c, d)
    return c * t / d + b
end

function M.inQuad(t, b, c, d)
    return c * pow(t / d, 2) + b
end

function M.outQuad(t, b, c, d)
    t = t / d
    return -c * t * (t - 2) + b
end

function M.inOutQuad(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 2) + b
    end
    return -c / 2 * ((t - 1) * (t - 3) - 1) + b
end

function M.outInQuad(t, b, c, d)
    if t < d / 2 then
        return M.outQuad(t * 2, b, c / 2, d)
    end
    return M.inQuad((t * 2) - d, b + c / 2, c / 2, d)
end

function M.inCubic (t, b, c, d)
    return c * pow(t / d, 3) + b
end

function M.outCubic(t, b, c, d)
    return c * (pow(t / d - 1, 3) + 1) + b
end

function M.inOutCubic(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * t * t * t + b
    end
    t = t - 2
    return c / 2 * (t * t * t + 2) + b
end

function M.outInCubic(t, b, c, d)
    if t < d / 2 then
        return M.outCubic(t * 2, b, c / 2, d)
    end
    return M.inCubic((t * 2) - d, b + c / 2, c / 2, d)
end

function M.inQuart(t, b, c, d)
    return c * pow(t / d, 4) + b
end

function M.outQuart(t, b, c, d)
    return -c * (pow(t / d - 1, 4) - 1) + b
end

function M.inOutQuart(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 4) + b
    end
    return -c / 2 * (pow(t - 2, 4) - 2) + b
end

function M.outInQuart(t, b, c, d)
    if t < d / 2 then
        return M.outQuart(t * 2, b, c / 2, d)
    end
    return M.inQuart((t * 2) - d, b + c / 2, c / 2, d)
end

function M.inQuint(t, b, c, d)
    return c * pow(t / d, 5) + b
end

function M.outQuint(t, b, c, d)
    return c * (pow(t / d - 1, 5) + 1) + b
end

function M.inOutQuint(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 5) + b
    end
    return c / 2 * (pow(t - 2, 5) + 2) + b
end

function M.outInQuint(t, b, c, d)
    if t < d / 2 then
        return M.outQuint(t * 2, b, c / 2, d)
    end
    return M.inQuint((t * 2) - d, b + c / 2, c / 2, d)
end

function M.inSine(t, b, c, d)
    return -c * cos(t / d * (pi / 2)) + c + b
end

function M.outSine(t, b, c, d)
    return c * sin(t / d * (pi / 2)) + b
end

function M.inOutSine(t, b, c, d)
    return -c / 2 * (cos(pi * t / d) - 1) + b
end

function M.outInSine(t, b, c, d)
    if t < d / 2 then
        return M.outSine(t * 2, b, c / 2, d)
    end
    return M.inSine((t * 2) -d, b + c / 2, c / 2, d)
end

function M.inExpo(t, b, c, d)
    if t == 0 then return b end
    return c * pow(2, 10 * (t / d - 1)) + b - c * 0.001
end

function M.outExpo(t, b, c, d)
    if t == d then return b + c end
    return c * 1.001 * (-pow(2, -10 * t / d) + 1) + b
end

function M.inOutExpo(t, b, c, d)
    if t == 0 then return b end
    if t == d then return b + c end
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(2, 10 * (t - 1)) + b - c * 0.0005 end
    return c / 2 * 1.0005 * (-pow(2, -10 * (t - 1)) + 2) + b
end

function M.outInExpo(t, b, c, d)
    if t < d / 2 then
        return M.outExpo(t * 2, b, c / 2, d)
    end
    return M.inExpo((t * 2) - d, b + c / 2, c / 2, d)
end


function M.inCirc(t, b, c, d)
    return -c * (sqrt(1 - pow(t / d, 2)) - 1) + b
end

function M.outCirc(t, b, c, d)
    return c * sqrt(1 - pow(t / d - 1, 2)) + b
end

function M.inOutCirc(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return -c / 2 * (sqrt(1 - t * t) - 1) + b
    end
    t = t - 2
    return c / 2 * (sqrt(1 - t * t) + 1) + b
end

function M.outInCirc(t, b, c, d)
    if t < d / 2 then
        return M.outCirc(t * 2, b, c / 2, d)
    end
    return M.inCirc((t * 2) - d, b + c / 2, c / 2, d)
end

function M.computePas(p,a,c,d)
    p, a = p or d * 0.3, a or 0
    if a < abs(c) then
        return p, c, p / 4
    end 
    return p, a, p / (2 * pi) * asin(c/a) 
end

function M.inElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d
    if t == 1  then return b + c end
    p,a,s = M.computePas(p,a,c,d)
    t = t - 1
    return -(a * pow(2, 10 * t) * sin((t * d - s) * (2 * pi) / p)) + b
end

function M.outElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d
    if t == 1 then return b + c end
    p,a,s = M.computePas(p,a,c,d)
    return a * pow(2, -10 * t) * sin((t * d - s) * (2 * pi) / p) + c + b
end

function M.inOutElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d * 2
    if t == 2 then return b + c end
    p,a,s = M.computePas(p,a,c,d)
    t = t - 1
    if t < 0 then return -0.5 * (a * pow(2, 10 * t) * sin((t * d - s) * (2 * pi) / p)) + b end
    return a * pow(2, -10 * t) * sin((t * d - s) * (2 * pi) / p ) * 0.5 + c + b
end

function M.outInElastic(t, b, c, d, a, p)
    if t < d / 2 then return M.outElastic(t * 2, b, c / 2, d, a, p) end
    return M.inElastic((t * 2) - d, b + c / 2, c / 2, d, a, p)
end

function M.inBack(t, b, c, d, s)
    s = s or 1.70158
    t = t / d
    return c * t * t * ((s + 1) * t - s) + b
end

function M.outBack(t, b, c, d, s)
    s = s or 1.70158
    t = t / d - 1
    return c * (t * t * ((s + 1) * t + s) + 1) + b
end

function M.inOutBack(t, b, c, d, s)
    s = (s or 1.70158) * 1.525
    t = t / d * 2
    if t < 1 then return c / 2 * (t * t * ((s + 1) * t - s)) + b end
    t = t - 2
    return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b
end

function M.outInBack(t, b, c, d, s)
    if t < d / 2 then return M.outBack(t * 2, b, c / 2, d, s) end
    return M.inBack((t * 2) - d, b + c / 2, c / 2, d, s)
end

function M.outBounce(t, b, c, d)
    t = t / d
    if t < 1 / 2.75 then return c * (7.5625 * t * t) + b end
    if t < 2 / 2.75 then
        t = t - (1.5 / 2.75)
        return c * (7.5625 * t * t + 0.75) + b
    elseif t < 2.5 / 2.75 then
        t = t - (2.25 / 2.75)
        return c * (7.5625 * t * t + 0.9375) + b
    end
    t = t - (2.625 / 2.75)
    return c * (7.5625 * t * t + 0.984375) + b
end

function M.inBounce(t, b, c, d) return c - M.outBounce(d - t, 0, c, d) + b end

function M.inOutBounce(t, b, c, d)
  if t < d / 2 then return M.inBounce(t * 2, 0, c, d) * 0.5 + b end
  return M.outBounce(t * 2 - d, 0, c, d) * 0.5 + c * .5 + b
end

function M.outInBounce(t, b, c, d)
  if t < d / 2 then return M.outBounce(t * 2, b, c / 2, d) end
  return M.inBounce((t * 2) - d, b + c / 2, c / 2, d)
end

-- 2 dimensional vector

do
    FFI.cdef[[
        typedef struct {
            double x, y;
        } Vector2;
    ]]

    local ffi_istype = FFI.istype

    local rawnew = FFI.typeof('Vector2')
    local function new(x, y)
        x = x or 0
        y = y or x
        return rawnew(x, y)
    end

    local sqrt, sin, cos, atan2 = M.sqrt, M.sin, M.cos, M.atan2
    local estSin, estCos, estAtan2 = M.sinEst, M.cosEst, M.atan2Est

    local function magnitude(v)
        local x, y = v.x, v.y
        return sqrt(x * x + y * y)
    end

    local function length2(v)
        local x, y = v.x, v.y
        return x * x + y * y
    end

    local function distance(a, b)
        local x, y = b.x - a.x, b.y - a.y
        return sqrt(x * x + y * y)
    end

    local function distance2(a, b)
        local x, y = b.x - a.x, b.y - a.y
        return x * x + y * y
    end

    local function dot(a, b)
        return a.x * b.x + a.y * b.y
    end

    local function normalize(v)
        return v / magnitude(v)
    end

    local function reflect(incident, normal)
        return incident - 2 * dot(normal, incident) * normal
    end

    local function refract(incident, normal, eta)
        local ndi = dot(normal, incident)
        local k = 1 - eta * eta * (1 - ndi * ndi)
        if k < 0 then
            return rawnew(0, 0)
        else
            return eta * incident - (eta * ndi + sqrt(k)) * normal
        end
    end

    local function rotate(v, a)
        local x, y = v.x, v.y
        return rawnew(
            x * cos(a) - y * sin(a),
            y * cos(a) + x * sin(a)
        )
    end

    local function estRotate(v, a)
        local x, y = v.x, v.y
        return rawnew(
            x * estCos(a) - y * estSin(a),
            y * estCos(a) + x * estSin(a)
        )
    end

    local function fromAngle(a)
        return rawnew(cos(a), sin(a))
    end

    local function estFromAngle(a)
        return rawnew(estCos(a), estSin(a))
    end

    local function toAngle(v)
        return atan2(v.y, v.x)
    end

    local function estToAngle(v)
        return estAtan2(v.y, v.x)
    end

    local function unpack(v)
        return v.x, v.y
    end

    local function clone(v)
        return rawnew(v.x, v.y)
    end

    local function clamp(x, min, max)
        local r = clone(x)
        r.x = M.clamp(r.x, min, max)
        r.y = M.clamp(r.y, min, max)
        return r
    end

    local ZERO = rawnew(0, 0)
    local ONE = rawnew(1, 1)
    local UNIT_X = rawnew(1, 0)
    local UNIT_Y = rawnew(0, 1)

    FFI.metatype('Vector2', {
        __add = function(a, b)
            if type(a) == 'number' then
                return rawnew(a + b.x, a + b.y)
            elseif type(b) == 'number' then
                return rawnew(a.x + b, a.y + b)
            else
                return rawnew(a.x + b.x, a.y + b.y)
            end
        end,
        __sub = function(a, b)
            if type(a) == 'number' then
                return rawnew(a - b.x, a - b.y)
            elseif type(b) == 'number' then
                return rawnew(a.x - b, a.y - b)
            else
                return rawnew(a.x - b.x, a.y - b.y)
            end
        end,
        __unm = function(v)
            return rawnew(-v.x, -v.y)
        end,
        __mul = function(a, b)
            if type(a) == 'number' then
                return rawnew(a * b.x, a * b.y)
            elseif type(b) == 'number' then
                return rawnew(a.x * b, a.y * b)
            else
                return rawnew(a.x * b.x, a.y * b.y)
            end
        end,
        __div = function(a, b)
            if type(a) == 'number' then
                return rawnew(a / b.x, a / b.y)
            elseif type(b) == 'number' then
                return rawnew(a.x / b, a.y / b)
            else
                return rawnew(a.x / b.x, a.y / b.y)
            end
        end,
        __mod = function(a, b)
            if type(a) == 'number' then
                return rawnew(a % b.x, a % b.y)
            elseif type(b) == 'number' then
                return rawnew(a.x % b, a.y % b)
            else
                return rawnew(a.x % b.x, a.y % b.y)
            end
        end,
        __eq = function(a, b)
            local isVec2 = type(b) == 'cdata' and ffi_istype('Vector2', b)
            return isVec2 and a.x == b.x and a.y == b.y
        end,
        __len = magnitude,
        __tostring = function(v)
            return string.format('(%f, %f)', v.x, v.y)
        end
    })

    M.Vector2 = setmetatable({
        new = new,
        magnitude = magnitude,
        length2 = length2,
        distance = distance,
        distance2 = distance2,
        dot = dot,
        normalize = normalize,
        reflect = reflect,
        refract = refract,
        rotate = rotate,
        estRotate = estRotate,
        fromAngle = fromAngle,
        estFromAngle = estFromAngle,
        toAngle = toAngle,
        estToAngle = estToAngle,
        unpack = unpack,
        clone = clone,
        clamp = clamp,
        ZERO = ZERO,
        ONE = ONE,
        UNIT_X = UNIT_X,
        UNIT_Y = UNIT_Y
    }, {
        __call = function(_, x, y)
            return new(x, y)
        end
    })
end

-- 3 dimensional vector

do
    FFI.cdef[[
        typedef struct {
            double x, y, z;
        } Vector3;
    ]]

    local ffi_istype = FFI.istype

    local rawnew = FFI.typeof('Vector3')
    local function new(x, y, z)
        x = x or 0
        y = y or x
        z = z or y
        return rawnew(x, y, z)
    end

    local sqrt, sin, cos, atan2 = M.sqrt, M.sin, M.cos, M.atan2
    local estSin, estCos, estAtan2 = M.sinEst, M.cosEst, M.atan2Est

    local function magnitude(v)
        local x, y, z = v.x, v.y, v.z
        return sqrt(x * x + y * y + z * z)
    end

    local function length2(v)
        local x, y, z = v.x, v.y, v.z
        return x * x + y * y + z * z
    end

    local function distance(a, b)
        local x, y, z = b.x - a.x, b.y - a.y, b.z - a.z
        return sqrt(x * x + y * y + z * z)
    end

    local function distance2(a, b)
        local x, y, z = b.x - a.x, b.y - a.y, b.z - a.z
        return x * x + y * y + z * z
    end

    local function dot(a, b)
        return a.x * b.x + a.y * b.y + a.z * b.z
    end

    local function cross(a, b)
        return rawnew(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
    end

    local function normalize(v)
        return v / magnitude(v)
    end

    local function reflect(incident, normal)
        return incident - 2 * dot(normal, incident) * normal
    end

    local function refract(incident, normal, eta)
        local ndi = dot(normal, incident)
        local k = 1 - eta * eta * (1 - ndi * ndi)
        if k < 0 then
            return rawnew(0, 0)
        else
            return eta * incident - (eta * ndi + sqrt(k)) * normal
        end
    end

    local function rotate(v, q)
        local qxyz = new(q.x, q.y, q.z)
        local uv = cross(qxyz, v)
        local uuv = cross(qxyz, uv)
        return v + ((uv * q.w) + uuv) * 2
    end

    local function fromAngles(theta, phi)
        local st, sp, ct, cp = sin(theta), sin(phi), cos(theta), cos(phi)
        return rawnew(st * sp, ct, st * cp)
    end

    local function estFromAngles(theta, phi)
        local st, sp, ct, cp = estSin(theta), estSin(phi), estCos(theta), estCos(phi)
        return rawnew(st * sp, ct, st * cp)
    end

    local function unpack(v)
        return v.x, v.y, v.z
    end

    local function clone(v)
        return rawnew(v.x, v.y, v.z)
    end

    -- Gradually changes a vector towards a desired goal over time.
    --- @return Vector3[2] @the damped vector and the modified velocity
    local function smoothDamp(current, target, velocity, smoothTime, maxSpeed, deltaTime)
        -- Based on Game Programming Gems 4 Chapter 1.10
        local out_x, out_y, out_z = 0, 0, 0
        smoothTime = M.max(0.0001, smoothTime)
        local omega = 2 / smoothTime
        local x = omega * deltaTime
        local exp = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x)

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

        -- prevent overshooting

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

    local function clamp(x, min, max)
        local r = clone(x)
        r.x = M.clamp(r.x, min, max)
        r.y = M.clamp(r.y, min, max)
        r.z = M.clamp(r.z, min, max)
        return r
    end

    local ZERO = rawnew(0, 0, 0)
    local ONE = rawnew(1, 1, 1)
    local FORWARD = rawnew(0, 0, 1)
    local BACKWARD = rawnew(0, 0, -1)
    local UP = rawnew(0, 1, 0)
    local DOWN = rawnew(0, -1, 0)
    local LEFT = rawnew(-1, 0, 0)
    local RIGHT = rawnew(1, 0, 0)
    local UNIT_X = rawnew(1, 0, 0)
    local UNIT_Y = rawnew(0, 1, 0)
    local UNIT_Z = rawnew(0, 0, 1)

    FFI.metatype('Vector3', {
        __add = function(a, b)
            if type(a) == 'number' then
                return rawnew(a + b.x, a + b.y, a + b.z)
            elseif type(b) == 'number' then
                return rawnew(a.x + b, a.y + b, a.z + b)
            else
                return rawnew(a.x + b.x, a.y + b.y, a.z + b.z)
            end
        end,
        __sub = function(a, b)
            if type(a) == 'number' then
                return rawnew(a - b.x, a - b.y, a - b.z)
            elseif type(b) == 'number' then
                return rawnew(a.x - b, a.y - b, a.z - b)
            else
                return rawnew(a.x - b.x, a.y - b.y, a.z - b.z)
            end
        end,
        __unm = function(v)
            return rawnew(-v.x, -v.y, -v.z)
        end,
        __mul = function(a, b)
            if type(a) == 'number' then
                return rawnew(a * b.x, a * b.y, a * b.z)
            elseif type(b) == 'number' then
                return rawnew(a.x * b, a.y * b, a.z * b)
            elseif type(b) == 'cdata' and ffi_istype('Quaternion', b) then
                local ix = b.w * a.x + b.y * a.z - b.z * a.y
                local iy = b.w * a.y + b.z * a.x - b.x * a.z
                local iz = b.w * a.z + b.x * a.y - b.y * a.x
                local iw = - b.x * a.x - b.y * a.y - b.z * a.z
                local x = ix * b.w + iw * - b.x + iy * - b.z - iz * - b.y
                local y = iy * b.w + iw * - b.y + iz * - b.x - ix * - b.z
                local z = iz * b.w + iw * - b.z + ix * - b.y - iy * - b.x
                return rawnew(x, y, z)
            else
                return rawnew(a.x * b.x, a.y * b.y, a.z * b.z)
            end
        end,
        __div = function(a, b)
            if type(a) == 'number' then
                return rawnew(a / b.x, a / b.y, a / b.z)
            elseif type(b) == 'number' then
                return rawnew(a.x / b, a.y / b, a.z / b)
            else
                return rawnew(a.x / b.x, a.y / b.y, a.z / b.z)
            end
        end,
        __mod = function(a, b)
            if type(a) == 'number' then
                return rawnew(a % b.x, a % b.y, a % b.z)
            elseif type(b) == 'number' then
                return rawnew(a.x % b, a.y % b, a.z % b)
            else
                return rawnew(a.x % b.x, a.y % b.y, a.z % b.z)
            end
        end,
        __eq = function(a, b)
            local isVec3 = type(b) == 'cdata' and ffi_istype('Vector3', b)
            return isVec3 and a.x == b.x and a.y == b.y and a.z == b.z
        end,
        __len = magnitude,
        __tostring = function(v)
            return string.format('(%f, %f, %f)', v.x, v.y, v.z)
        end
    })

    M.Vector3 = setmetatable({
        new = new,
        magnitude = magnitude,
        length2 = length2,
        distance = distance,
        distance2 = distance2,
        dot = dot,
        cross = cross,
        normalize = normalize,
        reflect = reflect,
        refract = refract,
        rotate = rotate,
        fromAngles = fromAngles,
        estFromAngles = estFromAngles,
        unpack = unpack,
        clone = clone,
        smoothDamp = smoothDamp,
        clamp = clamp,
        ZERO = ZERO,
        ONE = ONE,
        FORWARD = FORWARD,
        BACKWARD = BACKWARD,
        UP = UP,
        DOWN = DOWN,
        LEFT = LEFT,
        RIGHT = RIGHT,
        UNIT_X = UNIT_X,
        UNIT_Y = UNIT_Y,
        UNIT_Z = UNIT_Z
    }, {
        __call = function(_, x, y, z)
            return new(x, y, z)
        end
    })
end

-- 4 dimensional quaternion - can also be used as vector 4

do
    FFI.cdef[[
        typedef struct {
            double x, y, z, w;
        } Quaternion;
    ]]
        
    local ffi_istype = FFI.istype

    local rawnew = FFI.typeof('Quaternion')
    local function new(x, y, z, w)
        if x and y and z then
            if w then
                return rawnew(x, y, z, w)
            else
                return rawnew(x, y, z, 0)
            end
        else
            return rawnew(0, 0, 0, 1)
        end
    end

    local sqrt, sin, cos, atan2 = M.sqrt, M.sin, M.cos, M.atan2
    local estSin, estCos, estAtan2 = M.sinEst, M.cosEst, M.atan2Est

    local function magnitude(q)
        local x, y, z, w = q.x, q.y, q.z, q.w
        return sqrt(x * x + y * y + z * z + w * w)
    end

    local function normalize(q)
        local len = #q
        return rawnew(q.x / len, q.y / len, q.z / len, q.w / len)
    end

    local function inverse(q)
        return rawnew(-q.x, -q.y, -q.z, q.w)
    end

    local function dot(a, b)
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w
    end

    local function slerp(a, b, i)
        if a == b then return a end
        
        local cosHalfTheta = dot(a, b)
        local halfTheta = acos(cosHalfTheta)
        local sinHalfTheta = sqrt(1 - cosHalfTheta^2)
        
        return a * (sin((1 - i) * halfTheta) / sinHalfTheta) + b * (sin(i * halfTheta) / sinHalfTheta)
    end

    local function estSlerp(a, b, i)
        if a == b then return a end
        
        local cosHalfTheta = dot(a, b)
        local halfTheta = estAcos(cosHalfTheta)
        local sinHalfTheta = sqrt(1 - cosHalfTheta*cosHalfTheta)
        
        return a * (estSin((1 - i) * halfTheta) / sinHalfTheta) + b * (estSin(i * halfTheta) / sinHalfTheta)
    end

    local function fromAxisAngle(v)
        local angle = #v
        if angle == 0 then return rawnew(0, 0, 0, 1) end
        local axis = v / angle
        local s, c = sin(angle / 2), cos(angle / 2)
        return normalize(new(axis.x * s, axis.y * s, axis.z * s, c))
    end

    local function estFromAxisAngle(v)
        local angle = #v
        if angle == 0 then return rawnew(0, 0, 0, 1) end
        local axis = v / angle
        local s, c = estSin(angle / 2), estCos(angle / 2)
        return normalize(new(axis.x * s, axis.y * s, axis.z * s, c))
    end

    local function fromRollPitchYaw(pitch, yaw, roll)
        local hp = pitch * 0.5
        local cp = M.cos(hp)
        local sp = M.sin(hp)
        local hy = yaw * 0.5
        local cy = M.cos(hy)
        local sy = M.sin(hy)
        local hr = roll * 0.5
        local cr = M.cos(hr)
        local sr = M.sin(hr)
        local x = cr * sp * cy + sr * cp * sy
        local y = cr * cp * sy - sr * sp * cy
        local z = sr * cp * cy - cr * sp * sy
        local w = cr * cp * cy + sr * sp * sy
        return M.Quaternion(x, y, z, w)
    end

    local function estFromRollPitchYaw(pitch, yaw, roll)
        local hp = pitch * 0.5
        local cp = M.cosEst(hp)
        local sp = M.sinEst(hp)
        local hy = yaw * 0.5
        local cy = M.cosEst(hy)
        local sy = M.sinEst(hy)
        local hr = roll * 0.5
        local cr = M.cosEst(hr)
        local sr = M.sinEst(hr)
        local x = cr * sp * cy + sr * cp * sy
        local y = cr * cp * sy - sr * sp * cy
        local z = sr * cp * cy - cr * sp * sy
        local w = cr * cp * cy + sr * sp * sy
        return M.Quaternion(x, y, z, w)
    end

    local function unpack(q)
        return q.x, q.y, q.z, q.w
    end

    local function clone(q)
        return rawnew(q.x, q.y, q.z, q.w)
    end

    local ZERO = rawnew(0, 0, 0, 0)
    local IDENTITY = rawnew(0, 0, 0, 1)

    FFI.metatype('Quaternion', {
        __unm = function(q)
            return rawnew(-q.x, -q.y, -q.z, -q.w)
        end,
        __mul = function(a, b)
            local isQuat = type(b) == 'cdata' and ffi_istype('Quaternion', b)
            if isQuat then
                local x = a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y
                local y = a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z
                local z = a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x
                local w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
                return rawnew(x, y, z, w)
            else
                return rawnew(a.x * b, a.y * b, a.z * b, a.w * b)
            end
        end,
        __add = function(a, b)
            return rawnew(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w)
        end,
        __eq = function(a, b)
            local isQuat = type(b) == 'cdata' and ffi_istype('Quaternion', b)
            return isQuat and a.x == b.x and a.y == b.y and a.z == b.z and a.w == b.w
        end,
        __len = magnitude,
        __tostring = function(q)
            return string.format('(%f, %f, %f, %f)', q.x, q.y, q.z, q.w)
        end
    })

    M.Quaternion = setmetatable({
        new = new,
        magnitude = magnitude,
        normalize = normalize,
        inverse = inverse,
        dot = dot,
        slerp = slerp,
        estSlerp = estSlerp,
        fromAxisAngle = fromAxisAngle,
        estFromAxisAngle = estFromAxisAngle,
        fromRollPitchYaw = fromRollPitchYaw,
        estFromRollPitchYaw = estFromRollPitchYaw,
        unpack = unpack,
        clone = clone,
        ZERO = ZERO,
        IDENTITY = IDENTITY
    }, {
        __call = function(_, x, y, z, w)
            return new(x, y, z, w)
        end
    })
end

function M.randomPointWithinUnitSphere(radius)
    local theta = M.TAU * M.random()
    local phi = M.acos(M.random() * 2 - 1)
    local x = radius * M.sin(phi) * M.cos(theta)
    local y = radius * M.sin(phi) * M.sin(theta)
    local z = radius * M.cos(phi)
    return M.Vector3(x, y, z)
end

return M
