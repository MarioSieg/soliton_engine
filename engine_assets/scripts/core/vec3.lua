----------------------------------------------------------------------------
-- Soliton Engine Vector3 gmath Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------
-- Vector3 which is also used RGB color.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local bit = require 'bit'

local istype = ffi.istype
local rawnew = ffi.typeof('lua_vec3')
local sqrt, cos, sin, atan2, min, max, random = math.sqrt, math.cos, math.sin, math.atan2, math.min, math.max, math.random
local band, bor, bxor, lshift, rshift = bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift

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

local function _from_bgra(bgra)
    local b = band(bgra, 255) / 255.0
    local g = band(rshift(bgra, 8), 255) / 255.0
    local r = band(rshift(bgra, 16), 255) / 255.0
    return rawnew(r, g, b)
end

colors = {
    zero = _from_bgra(0x00000000),
    transparent = _from_bgra(0x00000000),
    aliceblue = _from_bgra(0xfff0f8ff),
    antiquewhite = _from_bgra(0xfffaebd7),
    aqua = _from_bgra(0xff00ffff),
    aquamarine = _from_bgra(0xff7fffd4),
    azure = _from_bgra(0xfff0ffff),
    beige = _from_bgra(0xfff5f5dc),
    bisque = _from_bgra(0xffffe4c4),
    black = _from_bgra(0xff000000),
    blanchedalmond = _from_bgra(0xffffebcd),
    blue = _from_bgra(0xff0000ff),
    blueviolet = _from_bgra(0xff8a2be2),
    brown = _from_bgra(0xffa52a2a),
    burlywood = _from_bgra(0xffdeb887),
    cadetblue = _from_bgra(0xff5f9ea0),
    chartreuse = _from_bgra(0xff7fff00),
    chocolate = _from_bgra(0xffd2691e),
    coral = _from_bgra(0xffff7f50),
    cornflowerblue = _from_bgra(0xff6495ed),
    cornsilk = _from_bgra(0xfffff8dc),
    crimson = _from_bgra(0xffdc143c),
    cyan = _from_bgra(0xff00ffff),
    darkblue = _from_bgra(0xff00008b),
    darkcyan = _from_bgra(0xff008b8b),
    darkgoldenrod = _from_bgra(0xffb8860b),
    darkgray = _from_bgra(0xffa9a9a9),
    darkgreen = _from_bgra(0xff006400),
    darkkhaki = _from_bgra(0xffbdb76b),
    darkmagenta = _from_bgra(0xff8b008b),
    darkolivegreen = _from_bgra(0xff556b2f),
    darkorange = _from_bgra(0xffff8c00),
    darkorchid = _from_bgra(0xff9932cc),
    darkred = _from_bgra(0xff8b0000),
    darksalmon = _from_bgra(0xffe9967a),
    darkseagreen = _from_bgra(0xff8fbc8b),
    darkslateblue = _from_bgra(0xff483d8b),
    darkslategray = _from_bgra(0xff2f4f4f),
    darkturquoise = _from_bgra(0xff00ced1),
    darkviolet = _from_bgra(0xff9400d3),
    deeppink = _from_bgra(0xffff1493),
    deepskyblue = _from_bgra(0xff00bfff),
    dimgray = _from_bgra(0xff696969),
    dodgerblue = _from_bgra(0xff1e90ff),
    firebrick = _from_bgra(0xffb22222),
    floralwhite = _from_bgra(0xfffffaf0),
    forestgreen = _from_bgra(0xff228b22),
    fuchsia = _from_bgra(0xffff00ff),
    gainsboro = _from_bgra(0xffdcdcdc),
    ghostwhite = _from_bgra(0xfff8f8ff),
    gold = _from_bgra(0xffffd700),
    goldenrod = _from_bgra(0xffdaa520),
    gray = _from_bgra(0xff808080),
    green = _from_bgra(0xff008000),
    greenyellow = _from_bgra(0xffadff2f),
    honeydew = _from_bgra(0xfff0fff0),
    hotpink = _from_bgra(0xffff69b4),
    indianred = _from_bgra(0xffcd5c5c),
    indigo = _from_bgra(0xff4b0082),
    ivory = _from_bgra(0xfffffff0),
    khaki = _from_bgra(0xfff0e68c),
    lavender = _from_bgra(0xffe6e6fa),
    lavenderblush = _from_bgra(0xfffff0f5),
    lawngreen = _from_bgra(0xff7cfc00),
    lemonchiffon = _from_bgra(0xfffffacd),
    lightblue = _from_bgra(0xffadd8e6),
    lightcoral = _from_bgra(0xfff08080),
    lightcyan = _from_bgra(0xffe0ffff),
    lightgoldenrodyellow = _from_bgra(0xfffafad2),
    lightgray = _from_bgra(0xffd3d3d3),
    lightgreen = _from_bgra(0xff90ee90),
    lightpink = _from_bgra(0xffffb6c1),
    lightsalmon = _from_bgra(0xffffa07a),
    lightseagreen = _from_bgra(0xff20b2aa),
    lightskyblue = _from_bgra(0xff87cefa),
    lightslategray = _from_bgra(0xff778899),
    lightsteelblue = _from_bgra(0xffb0c4de),
    lightyellow = _from_bgra(0xffffffe0),
    lime = _from_bgra(0xff00ff00),
    limegreen = _from_bgra(0xff32cd32),
    linen = _from_bgra(0xfffaf0e6),
    magenta = _from_bgra(0xffff00ff),
    maroon = _from_bgra(0xff800000),
    mediumaquamarine = _from_bgra(0xff66cdaa),
    mediumblue = _from_bgra(0xff0000cd),
    mediumorchid = _from_bgra(0xffba55d3),
    mediumpurple = _from_bgra(0xff9370db),
    mediumseagreen = _from_bgra(0xff3cb371),
    mediumslateblue = _from_bgra(0xff7b68ee),
    mediumspringgreen = _from_bgra(0xff00fa9a),
    mediumturquoise = _from_bgra(0xff48d1cc),
    mediumvioletred = _from_bgra(0xffc71585),
    midnightblue = _from_bgra(0xff191970),
    mintcream = _from_bgra(0xfff5fffa),
    mistyrose = _from_bgra(0xffffe4e1),
    moccasin = _from_bgra(0xffffe4b5),
    navajowhite = _from_bgra(0xffffdead),
    navy = _from_bgra(0xff000080),
    oldlace = _from_bgra(0xfffdf5e6),
    olive = _from_bgra(0xff808000),
    olivedrab = _from_bgra(0xff6b8e23),
    orange = _from_bgra(0xffffa500),
    orangered = _from_bgra(0xffff4500),
    orchid = _from_bgra(0xffda70d6),
    palegoldenrod = _from_bgra(0xffeee8aa),
    palegreen = _from_bgra(0xff98fb98),
    paleturquoise = _from_bgra(0xffafeeee),
    palevioletred = _from_bgra(0xffdb7093),
    papayawhip = _from_bgra(0xffffefd5),
    peachpuff = _from_bgra(0xffffdab9),
    peru = _from_bgra(0xffcd853f),
    pink = _from_bgra(0xffffc0cb),
    plum = _from_bgra(0xffdda0dd),
    powderblue = _from_bgra(0xffb0e0e6),
    purple = _from_bgra(0xff800080),
    red = _from_bgra(0xffff0000),
    rosybrown = _from_bgra(0xffbc8f8f),
    royalblue = _from_bgra(0xff4169e1),
    saddlebrown = _from_bgra(0xff8b4513),
    salmon = _from_bgra(0xfffa8072),
    sandybrown = _from_bgra(0xfff4a460),
    seagreen = _from_bgra(0xff2e8b57),
    seashell = _from_bgra(0xfffff5ee),
    sienna = _from_bgra(0xffa0522d),
    silver = _from_bgra(0xffc0c0c0),
    skyblue = _from_bgra(0xff87ceeb),
    slateblue = _from_bgra(0xff6a5acd),
    slategray = _from_bgra(0xff708090),
    snow = _from_bgra(0xfffffafa),
    springgreen = _from_bgra(0xff00ff7f),
    steelblue = _from_bgra(0xff4682b4),
    tan = _from_bgra(0xffd2b48c),
    teal = _from_bgra(0xff008080),
    thistle = _from_bgra(0xffd8bfd8),
    tomato = _from_bgra(0xffff6347),
    turquoise = _from_bgra(0xff40e0d0),
    violet = _from_bgra(0xffee82ee),
    wheat = _from_bgra(0xfff5deb3),
    white = _from_bgra(0xffffffff),
    whitesmoke = _from_bgra(0xfff5f5f5),
    yellow = _from_bgra(0xffffff00),
    yellowgreen = _from_bgra(0xff9acd32)
}

local function new(x, y, z)
    x = x or 0.0
    y = y or x
    z = z or x
    return rawnew(x, y, z)
end

local function from_rgb(rgb)
    local r = band(rshift(rgb, 24), 255) / 255.0
    local g = band(rshift(rgb, 16), 255) / 255.0
    local b = band(rshift(rgb, 8), 255) / 255.0
    return rawnew(r, g, b)
end

local function to_rgb(v)
    local r = band(v.x * 255.0, 255)
    local g = band(v.y * 255.0, 255)
    local b = band(v.z * 255.0, 255)
    local rgb = r
    rgb = bor(rgb, lshift(g, 8))
    rgb = bor(rgb, lshift(b, 16))
    return rgb
end

local function adjust_contract(v, contrast)
    local r, g, b = v.x, v.y, v.z
    r = 0.5 + contrast*(r - 0.5)
    g = 0.5 + contrast*(g - 0.5)
    b = 0.5 + contrast*(b - 0.5)
    return rawnew(r, g, b)
end

local function adjust_saturation(v, saturation)
    local r, g, b = v.x, v.y, v.z
    local grey = r*0.2125 + g*0.7154 + b*0.0721
    r = grey + saturation*(r - grey)
    g = grey + saturation*(g - grey)
    b = grey + saturation*(b - grey)
    return rawnew(r, g, b)
end

local function _random_scalar_range(min, max)
    return min + random() * (max - min);
end

local function random_range(from, to)
    from = from or 0.0
    to = to or 1.0
    local x = _random_scalar_range(from, to)
    local y = _random_scalar_range(from, to)
    local z = _random_scalar_range(from, to)
    return rawnew(x, y, z)
end

local function random_range_xz(from, to, y)
    from = from or 0.0
    to = to or 1.0
    y = y or 0.0
    local x = _random_scalar_range(from, to)
    local z = _random_scalar_range(from, to)
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
    local exp = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x *x)
    local change_x = current.x - target.x
    local change_y = current.y - target.y
    local change_z = current.z - target.z
    local origin = target
    local max_delta = max_speed * smooth_t
    local max_delta_sq = max_delta ^ 2
    local sqr_mag = change_x * change_x + change_y * change_y + change_z * change_z
    if sqr_mag > max_delta_sq then
        local mag = sqrt(sqr_mag)
        change_x = change_x / mag * max_delta
        change_y = change_y / mag * max_delta
        change_z = change_z / mag * max_delta
    end
    target.x = current.x - change_x
    target.y = current.y - change_y
    target.z = current.z - change_z
    local temp_x = (velocity.x + omega * change_x) * delta_t
    local temp_y = (velocity.y + omega * change_y) * delta_t
    local temp_z = (velocity.z + omega * change_z) * delta_t
    velocity.x = (velocity.x - omega * temp_x) * exp
    velocity.y = (velocity.y - omega * temp_y) * exp
    velocity.z = (velocity.z - omega * temp_z) * exp
    out_x = target.x + (change_x + temp_x) * exp
    out_y = target.y + (change_y + temp_y) * exp
    out_z = target.z + (change_z + temp_z) * exp
    local orig_delta_x = origin.x - current.x
    local orig_delta_y = origin.y - current.y
    local orig_delta_z = origin.z - current.z
    local out_delta_x = out_x - origin.x
    local out_delta_y = out_y - origin.y
    local out_delta_z = out_z - origin.z
    if orig_delta_x * out_delta_x + orig_delta_y * out_delta_y + orig_delta_z * out_delta_z > 0 then
        out_x = origin.x
        out_y = origin.y
        out_z = origin.z
        velocity.x = (out_x - origin.x) / delta_t
        velocity.y = (out_y - origin.y) / delta_t
        velocity.z = (out_z - origin.z) / delta_t
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
    from_rgb = from_rgb,
    to_rgb = to_rgb,
    adjust_contract = adjust_contract,
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
