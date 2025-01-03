local app = require 'app'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local time = require 'time'
local input = require 'input'
local gmath = require 'gmath'
local scene = require 'scene'

local tex = scene.load_texture('/RES/textures/proto/texture_01.png')
print(tex)
print(tex:get_width())
print(tex:get_height())
print(tex:get_depth())
print(tex:get_mip_levels())
print(tex:get_array_size())
print(tex:is_cubemap())
print(tex:get_format())

local mesh = scene.load_mesh('/RES/meshes/column.obj')
print(mesh)
print(mesh:get_primitive_count())
print(mesh:get_vertex_count())
print(mesh:get_index_count())
print(mesh:get_min_bound())
print(mesh:get_max_bound())
print(mesh:has_32_bit_indices())
