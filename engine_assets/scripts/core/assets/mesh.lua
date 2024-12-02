-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- mesh Module - Implements the mesh asset.
--- @module mesh
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C
local mesh_import_flags = require 'detail.import_flags'

ffi.cdef [[
    __asset_id __lu_mesh_load(const char* path, uint32_t import_flags);
    uint32_t __lu_mesh_get_primitive_count(__asset_id id);
    uint32_t __lu_mesh_get_vertex_count(__asset_id id);
    uint32_t __lu_mesh_get_index_count(__asset_id id);
    __vec3 __lu_mesh_get_min_bound(__asset_id id);
    __vec3 __lu_mesh_get_max_bound(__asset_id id);
    bool __lu_mesh_has_32_bit_indices(__asset_id id);
]]

local invalid_id = 0x7fffffff

local mesh = {
    _id = 0, --  asset id (scene local)
    _path = '' -- asset path
}

function mesh:get_id() return self._id end
function mesh:get_path() return self._path end
function mesh:get_primitive_count() return cpp.__lu_mesh_get_primitive_count(self._id) end
function mesh:get_vertex_count() return cpp.__lu_mesh_get_vertex_count(self._id) end
function mesh:get_index_count() return cpp.__lu_mesh_get_index_count(self._id) end
function mesh:get_min_bound() return cpp.__lu_mesh_get_min_bound(self._id) end
function mesh:get_max_bound() return cpp.__lu_mesh_get_max_bound(self._id) end
function mesh:has_32_bit_indices() return cpp.__lu_mesh_has_32_bit_indices(self._id) end
function mesh:__tostring() return string.format('mesh: %s', self._path) end

function mesh:from_id(id, path)
    assert(type(id) == 'number' and id ~= invalid_id)
    local o = {}
    setmetatable(o, { __index = self })
    o._id = id
    o._path = path
    return o
end

function mesh:load(path, import_flags)
    assert(type(path) == 'string' and path ~= '')
    local id = cpp.__lu_mesh_load(path, import_flags or mesh_import_flags.default)
    if id == invalid_id then return nil end
    return self:from_id(id, path)
end

return mesh
