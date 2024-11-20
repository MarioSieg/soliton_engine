-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- texture Module - Implements the texture asset.
--- @module texture
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C
local formats = require 'detail.texture_format'

ffi.cdef [[
    interop_asset_id __lu_texture_load(const char* path);
    uint32_t __lu_texture_get_width(interop_asset_id id);
    uint32_t __lu_texture_get_height(interop_asset_id id);
    uint32_t __lu_texture_get_depth(interop_asset_id id);
    uint32_t __lu_texture_get_mip_levels(interop_asset_id id);
    uint32_t __lu_texture_get_array_size(interop_asset_id id);
    bool __lu_texture_is_cubemap(interop_asset_id id);
    uint32_t __lu_texture_get_format(interop_asset_id id);
]]

local invalid_id = 0x7fffffff

local texture = {
    _id = 0, --  asset id (scene local)
    _path = '' -- asset path
}

function texture:id() return self._id end
function texture:path() return self._path end
function texture:width() return cpp.__lu_texture_get_width(self._id) end
function texture:height() return cpp.__lu_texture_get_height(self._id) end
function texture:depth() return cpp.__lu_texture_get_depth(self._id) end
function texture:mip_levels() return cpp.__lu_texture_get_mip_levels(self._id) end
function texture:array_size() return cpp.__lu_texture_get_array_size(self._id) end
function texture:is_cubemap() return cpp.__lu_texture_is_cubemap(self._id) end
function texture:format() return formats.formats_by_id[cpp.__lu_texture_get_format(self._id)] end
function texture:__tostring() return string.format('texture: %s', self._path) end

function texture:load(path)
    assert(type(path) == 'string' and path ~= '')
    local id = cpp.__lu_texture_load(path)
    if id == invalid_id then return nil end
    local o = {}
    setmetatable(o, { __index = self })
    o._id = id
    o._path = path
    return o
end

return texture
