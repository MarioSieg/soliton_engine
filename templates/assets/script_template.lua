local app = require 'app'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local time = require 'time'
local input = require 'input'
local gmath = require 'gmath'
local scene = require 'scene'

local tex = scene.load_texture('/RES/textures/proto/texture_01.png')
print(tex)
print(tex:width())
print(tex:height())
print(tex:depth())
print(tex:mip_levels())
print(tex:array_size())
print(tex:is_cubemap())
print(tex:format())

