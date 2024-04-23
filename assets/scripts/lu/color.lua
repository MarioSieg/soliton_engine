----------------------------------------------------------------------------
-- Lunam Engine Color Tools Module
--
-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local Color = {}

local min = math.min
local max = math.max
local atan = math.atan
local pi = math.pi
local sqrt = math.sqrt
local atan2 = math.atan2
local cos = math.cos
local sin = math.sin
local rad = math.rad
local abs = math.abs

local function getColorArgs(first, ...)
  if type(first) == "table" then
    return unpack(first)
  end
  return first, ...
end

--RGB to `colorspace`
function Color.RGBtoHSL(...)
  local r,g,b,a = getColorArgs(...)

  local min = min(r,g,b)
  local max = max(r,g,b)
  local delta_max = max-min

  local h,s,l

  l = (max+min)/2

  if (delta_max == 0) then
    h,s = 0,0
  else
    if l < 0.5 then
      s = delta_max / (max+min)
    else
      s = delta_max / (2-max-min)
    end

    local delta_r = (((max-r)/6) + (delta_max/2)) / delta_max
    local delta_g = (((max-g)/6) + (delta_max/2)) / delta_max
    local delta_b = (((max-b)/6) + (delta_max/2)) / delta_max

    if r == max then
      h = delta_b - delta_g
    elseif g == max then
      h = (1/3) + delta_r - delta_b
    elseif b == max then
      h = (2/3) + delta_b - delta_r
    end

    if h < 0 then h = h + 1 end
    if h > 1 then h = h - 1 end
  end

  return h,s,l,a
end

function Color.RGBtoHSV(...)
  local r,g,b,a = getColorArgs(...)

  local min = min(r,g,b)
  local max = max(r,g,b)
  local delta_max = max-min

  local h,s,v

  v = max

  if (delta_max == 0) then
    h,s = 0,0
  else
    s = delta_max / max

    local delta_r = (((max-r)/6) + (delta_max/2)) / delta_max
    local delta_g = (((max-g)/6) + (delta_max/2)) / delta_max
    local delta_b = (((max-b)/6) + (delta_max/2)) / delta_max

    if r == max then
      h = delta_b - delta_g
    elseif g == max then
      h = (1/3) + delta_r - delta_b
    elseif b == max then
      h = (2/3) + delta_b - delta_r
    end

    if h < 0 then h = h + 1 end
    if h > 1 then h = h - 1 end
  end

  return h,s,v,a
end

function Color.RGBtoXYZ(...)
  --(Observer = 2°, Illuminant = D65)
  local r,g,b,a = getColorArgs(...)


  if r > 0.04045 then
    r = ((r+0.055)/1.055)^2.4
  else
    r = r/12.92
  end
  if g > 0.04045 then
    g = ((g+0.055)/1.055)^2.4
  else
    g = g/12.92
  end
  if b > 0.04045 then
    b = ((b+0.055)/1.055)^2.4
  else
    b = b/12.92
  end

  r = r*100
  g = g*100
  b = b*100

  local x = r * 0.4124 + g * 0.3576 + b * 0.1805
  local y = r * 0.2126 + g * 0.7152 + b * 0.0722
  local z = r * 0.0193 + g * 0.1192 + b * 0.9505

  return x,y,z,a
end

function Color.RGBtoLab(...)
  return Color.XYZtoLab(Color.RGBtoXYZ(...))
end

function Color.RGBtoLCH(...)
  return Color.LabtoLCH(Color.RGBtoLab(...))
end

function Color.RGBtoLuv(...)
  return Color.XYZtoLuv(Color.RGBtoXYZ(...))
end

--`colorspace` to RGB
function Color.HSLtoRGB(...)
  local h,s,l,a = getColorArgs(...)
  h,s,l = h,s,l
  local r,g,b

  if s == 0 then
    r = l
    g = l
    b = l
  else
    local var1,var2

    if l < 0.5 then
      var2 = l*(1+s)
    else
      var2 = (l+s) - (s*l)
    end

    var1 = 2*l-var2

    local function huetorgb(v1,v2,vh)
      if vh < 0 then vh = vh+1 end
      if vh > 1 then vh = vh-1 end
      if 6*vh < 1 then return v1 + (v2-v1) * 6 * vh end
      if 2*vh < 1 then return v2 end
      if 3*vh < 2 then return v1 + (v2-v1) * ((2/3)-vh) * 6 end
      return v1
    end

    r = huetorgb(var1, var2, h + (1/3))
    g = huetorgb(var1, var2, h)
    b = huetorgb(var1, var2, h - (1/3))
  end

  return r,g,b,a
end

function Color.HSVtoRGB(...)
  local h,s,v,a = getColorArgs(...)

  local r,g,b

  if s == 0 then
    r = v
    g = v
    b = v
  else
    local varh,vari,var1,var2,var3
    varh = h*6
    if varh == 6 then varh = 0 end
    vari = math.floor(varh)
    var1 = v*(1-s)
    var2 = v*(1-s*(varh-vari))
    var3 = v*(1-s*(1-(varh-vari)))

    if vari == 0 then
      r = v
      g = var3
      b = var1
    elseif vari == 1 then
      r = var2
      g = v
      b = var1
    elseif vari == 2 then
      r = var1
      g = v
      b = var3
    elseif vari == 3 then
      r = var1
      g = var2
      b = v
    elseif vari == 4 then
      r = var3
      g = var1
      b = v
    else
      r = v
      g = var1
      b = var2
    end
  end

  return r,g,b,a
end

function Color.XYZtoRGB(...)
  --(Observer = 2°, Illuminant = D65)
  local x,y,z,a = getColorArgs(...)
  x,y,z = x/100,y/100,z/100
  local r,g,b

  r = x *  3.2406 + y * -1.5372 + z * -0.4986
  g = x * -0.9689 + y *  1.8758 + z *  0.0415
  b = x *  0.0557 + y * -0.2040 + z *  1.0570

  if r > 0.0031308 then
    r = 1.055*(r^(1/2.4))-0.055
  else
    r = r*12.92
  end
  if g > 0.0031308 then
    g = 1.055*(g^(1/2.4))-0.055
  else
    g = g*12.92
  end
  if b > 0.0031308 then
    b = 1.055*(b^(1/2.4))-0.055
  else
    b = b*12.92
  end

  return r,g,b,a
end

function Color.LabtoRGB(...)
  return Color.XYZtoRGB(Color.LabtoXYZ(...))
end

function Color.LCHtoRGB(...)
  return Color.LabtoRGB(Color.LCHtoLab(...))
end

function Color.LuvtoRGB(...)
  return Color.XYZtoRGB(Color.LuvtoXYZ(...))
end

--Other conversions
local refx,refy,refz = 95.047,100.000,108.883
local refu = (4 * refx) / (refx + (15 * refy) + (3 * refz))
local refv = (9 * refy) / (refx + (15 * refy) + (3 * refz))

function Color.XYZtoLab(...)
  local x,y,z,alpha = getColorArgs(...)
  local L,a,b
  x,y,z = x/refx,y/refy,z/refz
  if x > 0.008856 then
    x = x^(1/3)
  else
    x = (7.787*x) + (16/116)
  end
  if y > 0.008856 then
    y = y^(1/3)
  else
    y = (7.787*y) + (16/116)
  end
  if z > 0.008856 then
    z = z^(1/3)
  else
    z = (7.787*z) + (16/116)
  end

  L = (116*y) - 16
  a = 500*(x-y)
  b = 200*(y-z)
  return L,a,b,alpha
end

function Color.LabtoXYZ(...)
  local L,a,b,alpha = getColorArgs(...)

  local y = (L+16) / 116
  local x = a / 500 + y
  local z = y - b / 200
  if x^3 > 0.008856 then
    x = x^3
  else
    x = (x - 16 / 116) / 7.787
  end
  if y^3 > 0.008856 then
    y = y^3
  else
    y = (y - 16 / 116) / 7.787
  end
  if z^3 > 0.008856 then
    z = z^3
  else
    z = (z - 16 / 116) / 7.787
  end

  return refx*x,refy*y,refz*z,alpha
end

function Color.LabtoLCH(...)
  local L,a,b,alpha = getColorArgs(...)
  local C,H
  H = atan2(b,a)

  if H > 0 then
    H = (H / pi) * 180
  else
    H = 360 - ( abs(H) / pi) * 180
  end

  C = sqrt(a ^ 2 + b ^ 2)

  return L,C,H
end

function Color.LCHtoLab(...)
  local L,C,H,alpha = getColorArgs(...)

  return L,cos(rad(H))*C,sin(rad(H))*C
end

function Color.XYZtoLuv(...)
  local x,y,z,alpha = getColorArgs(...)
  local L,u,v
  u = (4 * x) / (x + (15 * y) + (3 * z))
  v = (9 * y) / (x + (15 * y) + (3 * z))
  y = y/100

  if y > 0.008856 then
    y = y ^ (1/3)
  else
    y = (7.787 * y) + (16 / 116)
  end

  L = (116 * y) - 16
  u = 13 * L * (u - refu)
  v = 13 * L * (v - refv)

  return L,u,v
end

function Color.LuvtoXYZ(...)
  local L,u,v,alpha = getColorArgs(...)
  local x,y,z

  y = (L + 16) / 116
  if y^3 > 0.008856 then
    y = y^3
  else
    y = (y - 16 / 116) / 7.787
  end

  u = u / (13 * L) + refu
  v = v / (13 * L) + refv

  y = y*100
  x = -(9 * y * u) / ((u - 4 ) * v - u * v)
  z = (9 * y - (15 * v * y) - (v * x)) / (3 * v)

  return x,y,z
end

--Manipulations:
function Color.lighten(amount, ...)
  local h,s,l,a = Color.RGBtoHSL(getColorArgs(...))
  return Color.HSLtoRGB(h,s,l+amount,a)
end
function Color.darken(amount, ...)
  local h,s,l,a = Color.RGBtoHSL(getColorArgs(...))
  return Color.HSLtoRGB(h,s,l-amount,a)
end
function Color.saturate(amount, ...)
  local h,s,v,a = Color.RGBtoHSV(getColorArgs(...))
  return Color.HSVtoRGB(h,s+amount,v,a)
end
function Color.desaturate(amount, ...)
  local h,s,v,a = Color.RGBtoHSV(getColorArgs(...))
  return Color.HSVtoRGB(h,s-amount,v,a)
end
function Color.hue(hue, ...)
  local h,s,l,a = Color.RGBtoHSL(getColorArgs(...))
  return Color.HSLtoRGB(hue,s,l,a)
end
function Color.invert(...)
  local r,g,b,a = getColorArgs(...)
  return 1-r, 1-g, 1-b, a
end
function Color.invertHue(...)
  local h,s,l,a = Color.RGBtoHSL(getColorArgs(...))
  return Color.HSLtoRGB(1-h,s,l,a)
end

--Spread
function Color.HSLSpread(count,hoffset,s,l,a)
  local incval = 1/count
  local colors = {}

  for i=0,count-1 do
    table.insert(colors,{Color.HSLtoRGB((i*incval+hoffset)%1,s,l,a)})
  end

  return colors
end
function Color.HSVSpread(count,hoffset,s,v,a)
  local incval = 1/count
  local colors = {}

  for i=0,count-1 do
    table.insert(colors,{Color.HSVtoRGB((i*incval+hoffset)%1,s,v,a)})
  end

  return colors
end
function Color.LCHSpread(count,l,c,hoffset,a)
  local incval = 360/count
  local colors = {}

  for i=0,count-1 do
    table.insert(colors,{Color.LCHtoRGB(l,c,(i*incval+hoffset)%360,a)})
  end

  return colors
end

--Wrap functions:
function Color.wrapHSV(fn)
  return function(...)
    return fn(Color.RGBtoHSV(...))
  end
end
function Color.wrapHSL(fn)
  return function(...)
    return fn(Color.RGBtoHSL(...))
  end
end
function Color.wrapXYZ(fn)
  return function(...)
    return fn(Color.RGBtoXYZ(...))
  end
end
function Color.wrapLab(fn)
  return function(...)
    return fn(Color.RGBtoLab(...))
  end
end
function Color.wrapLCH(fn)
  return function(...)
    return fn(Color.RGBtoLCH(...))
  end
end
function Color.wrapLuv(fn)
  return function(...)
    return fn(Color.RGBtoLuv(...))
  end
end

return Color
