-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local C = ffi.C

ffi.cdef[[
    bool __lu_com_transform_exists(lua_entity_id id);
    void __lu_com_transform_add(lua_entity_id id);
    void __lu_com_transform_remove(lua_entity_id id);

    void __lu_com_transform_set_pos(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_com_transform_get_pos(lua_entity_id id);
    void __lu_com_transform_set_rot(lua_entity_id id, double x, double y, double z, double w);
    lua_vec4 __lu_com_transform_get_rot(lua_entity_id id);
    void __lu_com_transform_set_scale(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_com_transform_get_scale(lua_entity_id id);
]]

local Transform = {
    _entity_id = nil,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o._entity_id = entity_id
        return o
    end,
    _exists = function(entity_id) return C.__lu_com_transform_exists(entity_id) end,
    remove = function(self) C.__lu_com_transform_remove(self._entity_id) end,

    setPosition = function(self, pos) C.__lu_com_transform_set_pos(self._entity_id, pos.x, pos.y, pos.z) end,
    getPosition = function(self) return C.__lu_com_transform_get_pos(self._entity_id) end,
    setRotation = function(self, rot) C.__lu_com_transform_set_rot(self._entity_id, rot.x, rot.y, rot.z, rot.w) end,
    getRotation = function(self) return C.__lu_com_transform_get_rot(self._entity_id) end,
    setScale = function(self, scale) C.__lu_com_transform_set_scale(self._entity_id, scale.x, scale.y, scale.z) end,
    getScale = function(self) return C.__lu_com_transform_get_scale(self._entity_id) end
}

return Transform
