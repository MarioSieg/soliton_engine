-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    bool __lu_com_mesh_renderer_exists(lua_entity_id id);
    void __lu_com_mesh_renderer_add(lua_entity_id id);
    void __lu_com_mesh_renderer_remove(lua_entity_id id);
]]

local camera = {
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
}

return camera
