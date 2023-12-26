-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local FFI = require 'ffi'

local pow, sin, cos, pi, sqrt, abs, asin = math.pow, math.sin, math.cos, math.pi, math.sqrt, math.abs, math.asin
local band, bor, bxor, lshift, rshift, arshift = bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift, bit.arshift

Math = {}

Math.E = 2.7182818284590452354 -- e
Math.LOG2E = 1.4426950408889634074 -- log_2 e
Math.LOG10E = 0.43429448190325182765 -- log_10 e
Math.LN2 = 0.69314718055994530942-- log_e 2
Math.LN10 = 2.30258509299404568402 -- log_e 10
Math.TAU = 6.28318530717958647692 -- pi * 2
Math.PI = 3.14159265358979323846 -- pi
Math.PI_DIV2 = 1.57079632679489661923 -- pi/2
Math.PI_DIV4 = 0.78539816339744830962 -- pi/4
Math.ONE_DIV_PI = 0.31830988618379067154 -- 1/pi
Math.TWO_DIV_PI = 0.63661977236758134308 -- 2/pi
Math.TWO_SQRTPI = 1.12837916709551257390 -- 2/sqrt(pi)
Math.SQRT2 = 1.41421356237309504880 -- sqrt(2)
Math.SQRT1_2 = 0.70710678118654752440 -- 1/sqrt(2)
Math.INFINITY = math.huge

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
function Math.pow(x, y) return x ^ y end
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
return Math.pow(2, Math.ceil(Math.log(x) / Math.LN2))
end

function Math.floorPowerOfTwo(x)
    return Math.pow(2, Math.floor(Math.log(x) / Math.LN2))
end

function Math.lerp(x, y, t)
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

function Math.intPow(x, n)
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

function Math.expEst(x)
    local xint, xfract = Math.modf(x)
    local exint = Math.intPow(Math.E, xint)
    local exfract = 1 + xfract + (xfract * xfract / 2) + (xfract * xfract * xfract / 6) + (xfract * xfract * xfract * xfract / 24)
    return exint * exfract
end

function Math.logEst(x)
    local xmant, xexp = Math.frexp(x)
    if xmant == 0.5 then
        return Math.LN2 * (xexp-1)
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
    return sum + Math.LN2 * (xexp - 1)
end
	
function Math.powEst(x, y)
    local yint, yfract = Math.modf(y)
    local xyint = Math.intPow(x, yint)
    local xyfract = Math.expEst(Math.logEst(x) * yfract)
    return xyint * xyfract -- x ^ (yint + yfract)
end

function Math.sinEst(x)
    local over = Math.floor(x / (Math.TAU / 2)) % 2 == 0 
    x = Math.TAU / 4 - x % (Math.TAU / 2)
    local abs = 1 - (20 * x * x) / (4 * x * x + Math.TAU * Math.TAU)
    return over and abs or -abs
end

function Math.cosEst(x)
    local over = Math.floor((Math.TAU / 4 - x) / (Math.TAU / 2)) % 2 == 0
    x = Math.TAU / 4 - (Math.TAU / 4 - x) % (Math.TAU/  2)
    local abs = 1 - (20 * x * x) / (4 * x * x + Math.TAU * Math.TAU)
    return over and abs or -abs
end

function Math.tanEst(x)
    return Math.sinEst(x) / Math.cosEst(x)
end

function Math.asinEst(x)
    local positiveX, x = x > 0, Math.abs(x)
    local resultForAbsoluteX = Math.TAU / 4 - Math.sqrt(Math.TAU / Math.TAU * (1 - x)) / (2 * Math.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX
end

function Math.acosEst(x)
    local positiveX, x = x > 0, Math.abs(x)
    local resultForAbsoluteX = Math.sqrt(Math.TAU * Math.TAU * (1 - x)) / (2 * Math.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX + Math.TAU / 2
end

function Math.atanEst(x)
    x = x / Math.sqrt(1 + x*x)
    local positiveX, x = x > 0, Math.abs(x)
    local resultForAbsoluteX = Math.TAU / 4 - Math.sqrt(Math.TAU * Math.TAU * (1 - x)) / (2 * Math.sqrt(x + 4))
    return positiveX and resultForAbsoluteX or -resultForAbsoluteX
end

function Math.atan2Est(y, x)
    if x == 0 and y == 0 then
        return 0
    end
    local theta = Math.atan(y/x)
    theta = x == 0 and Math.TAU / 4 * y / Math.abs(y) or x < 0 and theta + Math.TAU / 2 or theta
    theta = theta > Math.TAU / 2 and theta - Math.TAU or theta
    return theta
end

function Math.sinhEst(x)
    local ex = Math.expEst(x)
    return (ex - 1 / ex) / 2
end

function Math.coshEst(x)
    local ex = Math.expEst(x)
    return (ex + 1 / ex) / 2
end

function Math.tanhEst(x)
    local ex = Math.expEst(x)
    return (ex - 1 / ex) / (ex + 1 / ex)
end

-- https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/
function Math.inverseLerp(x, y, value)
    if x ~= y then
        return (value - x ) / (y - x)
    end
    return 0
end

-- http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
function Math.damp(x, y, lambda, dt)
    return Math.lerp(x, y, 1 - Math.expEst(-lambda * dt))
end

-- http://en.wikipedia.org/wiki/Smoothstep
function Math.smoothstep(x, min, max)
    if x <= min then return 0 end
    if x >= max then return 1 end
    x = (x - min) / (max - min)
    return x * x * (3 - 2 * x)
end

-- http://en.wikipedia.org/wiki/Smoothstep
function Math.smootherstep(x, min, max)
    if x <= min then return 0 end
    if x >= max then return 1 end
    x = (x - min) / (max - min)
    return x * x * x * (x * (x * 6 - 15) + 10)
end

function Math.saturate(x)
    return Math.max(0, Math.min(1, x))
end

function Math.cameraLinearize(depth, znear, zfar)
    return - zfar * znear / (depth * (zfar - znear) - zfar)
end

function Math.cameraSmoothstep(znear, zfar, depth)
    local x = Math.saturate((depth - znear) / (zfar - znear))
    return (x ^ 2) * (3 - 2 * x)
end

local seed = 0x12D687 -- Deterministic pseudo-random float in the interval [ 0, 1 ] (Mulberry32 generator)

function Math.seededRandom(s)
    seed = seed or s
    seed = seed + 0x6D2B79F5
    local t = seed
    t = bxor(t, arshift(t, 15)) * bor(t, 1)
    t = bxor(t, t + arshift(bxor(t, t), 7) * bor(t, 61))
    return arshift(bxor(t, t), 14) / 4294967296
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

function Math.randomRange(lower, greater)
    return lower + Math.random() * (greater - lower);
end

function Math.linear(t, b, c, d)
    return c * t / d + b
end

function Math.inQuad(t, b, c, d)
    return c * pow(t / d, 2) + b
end

function Math.outQuad(t, b, c, d)
    t = t / d
    return -c * t * (t - 2) + b
end

function Math.inOutQuad(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 2) + b
    end
    return -c / 2 * ((t - 1) * (t - 3) - 1) + b
end

function Math.outInQuad(t, b, c, d)
    if t < d / 2 then
        return Math.outQuad(t * 2, b, c / 2, d)
    end
    return Math.inQuad((t * 2) - d, b + c / 2, c / 2, d)
end

function Math.inCubic (t, b, c, d)
    return c * pow(t / d, 3) + b
end

function Math.outCubic(t, b, c, d)
    return c * (pow(t / d - 1, 3) + 1) + b
end

function Math.inOutCubic(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * t * t * t + b
    end
    t = t - 2
    return c / 2 * (t * t * t + 2) + b
end

function Math.outInCubic(t, b, c, d)
    if t < d / 2 then
        return Math.outCubic(t * 2, b, c / 2, d)
    end
    return Math.inCubic((t * 2) - d, b + c / 2, c / 2, d)
end

function Math.inQuart(t, b, c, d)
    return c * pow(t / d, 4) + b
end

function Math.outQuart(t, b, c, d)
    return -c * (pow(t / d - 1, 4) - 1) + b
end

function Math.inOutQuart(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 4) + b
    end
    return -c / 2 * (pow(t - 2, 4) - 2) + b
end

function Math.outInQuart(t, b, c, d)
    if t < d / 2 then
        return Math.outQuart(t * 2, b, c / 2, d)
    end
    return Math.inQuart((t * 2) - d, b + c / 2, c / 2, d)
end

function Math.inQuint(t, b, c, d)
    return c * pow(t / d, 5) + b
end

function Math.outQuint(t, b, c, d)
    return c * (pow(t / d - 1, 5) + 1) + b
end

function Math.inOutQuint(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(t, 5) + b
    end
    return c / 2 * (pow(t - 2, 5) + 2) + b
end

function Math.outInQuint(t, b, c, d)
    if t < d / 2 then
        return Math.outQuint(t * 2, b, c / 2, d)
    end
    return Math.inQuint((t * 2) - d, b + c / 2, c / 2, d)
end

function Math.inSine(t, b, c, d)
    return -c * cos(t / d * (pi / 2)) + c + b
end

function Math.outSine(t, b, c, d)
    return c * sin(t / d * (pi / 2)) + b
end

function Math.inOutSine(t, b, c, d)
    return -c / 2 * (cos(pi * t / d) - 1) + b
end

function Math.outInSine(t, b, c, d)
    if t < d / 2 then
        return Math.outSine(t * 2, b, c / 2, d)
    end
    return Math.inSine((t * 2) -d, b + c / 2, c / 2, d)
end

function Math.inExpo(t, b, c, d)
    if t == 0 then return b end
    return c * pow(2, 10 * (t / d - 1)) + b - c * 0.001
end

function Math.outExpo(t, b, c, d)
    if t == d then return b + c end
    return c * 1.001 * (-pow(2, -10 * t / d) + 1) + b
end

function Math.inOutExpo(t, b, c, d)
    if t == 0 then return b end
    if t == d then return b + c end
    t = t / d * 2
    if t < 1 then
        return c / 2 * pow(2, 10 * (t - 1)) + b - c * 0.0005 end
    return c / 2 * 1.0005 * (-pow(2, -10 * (t - 1)) + 2) + b
end

function Math.outInExpo(t, b, c, d)
    if t < d / 2 then
        return Math.outExpo(t * 2, b, c / 2, d)
    end
    return Math.inExpo((t * 2) - d, b + c / 2, c / 2, d)
end


function Math.inCirc(t, b, c, d)
    return -c * (sqrt(1 - pow(t / d, 2)) - 1) + b
end

function Math.outCirc(t, b, c, d)
    return c * sqrt(1 - pow(t / d - 1, 2)) + b
end

function Math.inOutCirc(t, b, c, d)
    t = t / d * 2
    if t < 1 then
        return -c / 2 * (sqrt(1 - t * t) - 1) + b
    end
    t = t - 2
    return c / 2 * (sqrt(1 - t * t) + 1) + b
end

function Math.outInCirc(t, b, c, d)
    if t < d / 2 then
        return Math.outCirc(t * 2, b, c / 2, d)
    end
    return Math.inCirc((t * 2) - d, b + c / 2, c / 2, d)
end

function Math.computePas(p,a,c,d)
    p, a = p or d * 0.3, a or 0
    if a < abs(c) then
        return p, c, p / 4
    end 
    return p, a, p / (2 * pi) * asin(c/a) 
end

function Math.inElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d
    if t == 1  then return b + c end
    p,a,s = Math.computePas(p,a,c,d)
    t = t - 1
    return -(a * pow(2, 10 * t) * sin((t * d - s) * (2 * pi) / p)) + b
end

function Math.outElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d
    if t == 1 then return b + c end
    p,a,s = Math.computePas(p,a,c,d)
    return a * pow(2, -10 * t) * sin((t * d - s) * (2 * pi) / p) + c + b
end

function Math.inOutElastic(t, b, c, d, a, p)
    local s
    if t == 0 then return b end
    t = t / d * 2
    if t == 2 then return b + c end
    p,a,s = Math.computePas(p,a,c,d)
    t = t - 1
    if t < 0 then return -0.5 * (a * pow(2, 10 * t) * sin((t * d - s) * (2 * pi) / p)) + b end
    return a * pow(2, -10 * t) * sin((t * d - s) * (2 * pi) / p ) * 0.5 + c + b
end

function Math.outInElastic(t, b, c, d, a, p)
    if t < d / 2 then return Math.outElastic(t * 2, b, c / 2, d, a, p) end
    return Math.inElastic((t * 2) - d, b + c / 2, c / 2, d, a, p)
end

function Math.inBack(t, b, c, d, s)
    s = s or 1.70158
    t = t / d
    return c * t * t * ((s + 1) * t - s) + b
end

function Math.outBack(t, b, c, d, s)
    s = s or 1.70158
    t = t / d - 1
    return c * (t * t * ((s + 1) * t + s) + 1) + b
end

function Math.inOutBack(t, b, c, d, s)
    s = (s or 1.70158) * 1.525
    t = t / d * 2
    if t < 1 then return c / 2 * (t * t * ((s + 1) * t - s)) + b end
    t = t - 2
    return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b
end

function Math.outInBack(t, b, c, d, s)
    if t < d / 2 then return Math.outBack(t * 2, b, c / 2, d, s) end
    return Math.inBack((t * 2) - d, b + c / 2, c / 2, d, s)
end

function Math.outBounce(t, b, c, d)
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

function Math.inBounce(t, b, c, d) return c - Math.outBounce(d - t, 0, c, d) + b end

function Math.inOutBounce(t, b, c, d)
  if t < d / 2 then return Math.inBounce(t * 2, 0, c, d) * 0.5 + b end
  return Math.outBounce(t * 2 - d, 0, c, d) * 0.5 + c * .5 + b
end

function Math.outInBounce(t, b, c, d)
  if t < d / 2 then return Math.outBounce(t * 2, b, c / 2, d) end
  return Math.inBounce((t * 2) - d, b + c / 2, c / 2, d)
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

    local sqrt, sin, cos, atan2 = Math.sqrt, Math.sin, Math.cos, Math.atan2
    local estSin, estCos, estAtan2 = Math.sinEst, Math.cosEst, Math.atan2Est

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
        r.x = Math.clamp(r.x, min, max)
        r.y = Math.clamp(r.y, min, max)
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

    Math.Vector2 = setmetatable({
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

    local sqrt, sin, cos, atan2 = Math.sqrt, Math.sin, Math.cos, Math.atan2
    local estSin, estCos, estAtan2 = Math.sinEst, Math.cosEst, Math.atan2Est

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
        smoothTime = Math.max(0.0001, smoothTime)
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
        r.x = Math.clamp(r.x, min, max)
        r.y = Math.clamp(r.y, min, max)
        r.z = Math.clamp(r.z, min, max)
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

    Math.Vector3 = setmetatable({
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

    local sqrt, sin, cos, atan2 = Math.sqrt, Math.sin, Math.cos, Math.atan2
    local estSin, estCos, estAtan2 = Math.sinEst, Math.cosEst, Math.atan2Est

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
        local cp = Math.cos(hp)
        local sp = Math.sin(hp)
        local hy = yaw * 0.5
        local cy = Math.cos(hy)
        local sy = Math.sin(hy)
        local hr = roll * 0.5
        local cr = Math.cos(hr)
        local sr = Math.sin(hr)
        local x = cr * sp * cy + sr * cp * sy
        local y = cr * cp * sy - sr * sp * cy
        local z = sr * cp * cy - cr * sp * sy
        local w = cr * cp * cy + sr * sp * sy
        return Math.Quaternion(x, y, z, w)
    end

    local function estFromRollPitchYaw(pitch, yaw, roll)
        local hp = pitch * 0.5
        local cp = Math.cosEst(hp)
        local sp = Math.sinEst(hp)
        local hy = yaw * 0.5
        local cy = Math.cosEst(hy)
        local sy = Math.sinEst(hy)
        local hr = roll * 0.5
        local cr = Math.cosEst(hr)
        local sr = Math.sinEst(hr)
        local x = cr * sp * cy + sr * cp * sy
        local y = cr * cp * sy - sr * sp * cy
        local z = sr * cp * cy - cr * sp * sy
        local w = cr * cp * cy + sr * sp * sy
        return Math.Quaternion(x, y, z, w)
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

    Math.Quaternion = setmetatable({
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

function Math.randomPointWithinUnitSphere(radius)
    local theta = Math.TAU * Math.random()
    local phi = Math.acos(Math.random() * 2 - 1)
    local x = radius * Math.sin(phi) * Math.cos(theta)
    local y = radius * Math.sin(phi) * Math.sin(theta)
    local z = radius * Math.cos(phi)
    return Math.Vector3(x, y, z)
end

return Math
