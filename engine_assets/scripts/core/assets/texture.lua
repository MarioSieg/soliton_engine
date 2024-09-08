-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- texture Module - Implements the texture asset.
--- @module texture
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    lua_asset_id __lu_texture_load(const char* path);
]]

local texture = {
    _id = 0, --  asset id (scene local)
    _path = '' -- asset path
}

function texture:id()
    return self._id
end

function texture:path()
    return self._path
end

function texture:is_valid()
    return self._id ~= 0x7fffffff
end

function texture:__tostring()
    return string.format('texture: %s', self._path)
end

function texture.load(resource_path)
    assert(type(resource_path) == 'string' and resource_path ~= '', 'resource_path must be a non-empty string')
    local o = {}
    setmetatable(o, { __index = self })
    o._id = cpp.__lu_texture_load(resource_path)
    o._path = resource_path
    return o
end

return texture
