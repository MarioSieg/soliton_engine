-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- mesh_renderer Module - Implements the mesh renderer component for rendering meshes.
--- @module mesh_renderer
------------------------------------------------------------------------------

local ffi = require 'ffi'
local bit = require 'bit'
local a_mesh = require 'assets.mesh'

local band, bor, bxor, bnot = bit.band, bit.bor, bit.bxor, bit.bnot
local cpp = ffi.C

ffi.cdef [[
    bool __lu_com_mesh_renderer_exists(__entity_id id);
    void __lu_com_mesh_renderer_add(__entity_id id);
    void __lu_com_mesh_renderer_remove(__entity_id id);

    uint32_t __lu_com_mesh_renderer_get_flags(__entity_id id);
    void __lu_com_mesh_renderer_set_flags(__entity_id id, uint32_t flags);
]]

render_flags = { -- Keep in sync with enum render_flags::$ in components.hpp
    none = 0,
    skip_rendering = 0x1,
    skip_frustum_culling = 0x2
}

local mesh_renderer = {
    _id = 0xdd559a3d,
    _entity_id = 0,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, { __index = self })
        o._entity_id = entity_id
        cpp.__lu_com_mesh_renderer_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return cpp.__lu_com_mesh_renderer_exists(entity_id) end,
    _remove = function(entity_id) cpp.__lu_com_mesh_renderer_remove(entity_id) end,

    get_flags = function(self) return cpp.__lu_com_mesh_renderer_get_flags(self._entity_id) end,
    set_flags = function(self, flags) cpp.__lu_com_mesh_renderer_set_flags(self._entity_id, flags) end,
    has_flag = function(self, flags) return band(self:get_flags(), flags) ~= 0 end,
    add_flag = function(self, flags) self:set_flags(bor(self:get_flags(), flags)) end,
    remove_flag = function(self, flags) self:set_flags(band(self:get_flags(), bnot(flags))) end,
    toggle_flag = function(self, flags) self:set_flags(bxor(self:get_flags(), flags)) end
}

return mesh_renderer
