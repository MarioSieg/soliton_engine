-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local cpp = ffi.C

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
    lua_vec3 __lu_com_transform_get_forward(lua_entity_id id);
    lua_vec3 __lu_com_transform_get_backward(lua_entity_id id);
    lua_vec3 __lu_com_transform_get_up(lua_entity_id id);
    lua_vec3 __lu_com_transform_get_down(lua_entity_id id);
    lua_vec3 __lu_com_transform_get_right(lua_entity_id id);
    lua_vec3 __lu_com_transform_get_left(lua_entity_id id);
]]

local transform = {
    _entity_id = 0,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o._entity_id = entity_id
        cpp.__lu_com_transform_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return cpp.__lu_com_transform_exists(entity_id) end,
    remove = function(self) cpp.__lu_com_transform_remove(self._entity_id) end,

    set_position = function(self, pos) cpp.__lu_com_transform_set_pos(self._entity_id, pos.x, pos.y, pos.z) end,
    get_position = function(self) return cpp.__lu_com_transform_get_pos(self._entity_id) end,
    set_rotation = function(self, rot) cpp.__lu_com_transform_set_rot(self._entity_id, rot.x, rot.y, rot.z, rot.w) end,
    get_rotation = function(self) return cpp.__lu_com_transform_get_rot(self._entity_id) end,
    set_scale = function(self, scale) cpp.__lu_com_transform_set_scale(self._entity_id, scale.x, scale.y, scale.z) end,
    get_scale = function(self) return cpp.__lu_com_transform_get_scale(self._entity_id) end,
    get_forward_dir = function(self) return cpp.__lu_com_transform_get_forward(self._entity_id) end,
    get_backward_dir = function(self) return cpp.__lu_com_transform_get_backward(self._entity_id) end,
    get_up_dir = function(self) return cpp.__lu_com_transform_get_up(self._entity_id) end,
    get_down_dir = function(self) return cpp.__lu_com_transform_get_down(self._entity_id) end,
    get_right_dir = function(self) return cpp.__lu_com_transform_get_right(self._entity_id) end,
    get_left_dir = function(self) return cpp.__lu_com_transform_get_left(self._entity_id) end,
}

return transform
