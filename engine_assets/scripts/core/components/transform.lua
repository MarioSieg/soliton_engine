-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- transform Module - Implements the transform component.
--- @module transform
------------------------------------------------------------------------------

local ffi = require 'ffi'
local utility = require 'utility'
local serializer = require 'detail.serializer'

local cpp = ffi.C

ffi.cdef [[
    bool __lu_com_transform_exists(__entity_id id);
    void __lu_com_transform_add(__entity_id id);
    void __lu_com_transform_remove(__entity_id id);

    void __lu_com_transform_set_pos(__entity_id id, double x, double y, double z);
    __vec3 __lu_com_transform_get_pos(__entity_id id);
    void __lu_com_transform_set_rot(__entity_id id, double x, double y, double z, double w);
    __vec4 __lu_com_transform_get_rot(__entity_id id);
    void __lu_com_transform_set_scale(__entity_id id, double x, double y, double z);
    __vec3 __lu_com_transform_get_scale(__entity_id id);
    __vec3 __lu_com_transform_get_forward(__entity_id id);
    __vec3 __lu_com_transform_get_backward(__entity_id id);
    __vec3 __lu_com_transform_get_up(__entity_id id);
    __vec3 __lu_com_transform_get_down(__entity_id id);
    __vec3 __lu_com_transform_get_right(__entity_id id);
    __vec3 __lu_com_transform_get_left(__entity_id id);
]]

local transform = {
    _id = 0xeed8df38,
    _entity_id = 0,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, { __index = self })
        o._entity_id = entity_id
        cpp.__lu_com_transform_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return cpp.__lu_com_transform_exists(entity_id) end,
    _remove = function(entity_id) cpp.__lu_com_transform_remove(entity_id) end,
    _serialize = function(self)
        return {
            position = serializer.serialize(self:get_position()),
            rotation = serializer.serialize(self:get_rotation()),
            scale = serializer.serialize(self:get_scale())
        }
    end,

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
