-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local C = ffi.C

ffi.cdef[[
    bool __lu_com_camera_exists(lua_entity_id id);
    void __lu_com_camera_add(lua_entity_id id);
    void __lu_com_camera_remove(lua_entity_id id);
    
    double __lu_com_camera_get_fov(lua_entity_id id);
    void __lu_com_camera_set_fov(lua_entity_id id, double fov);
    double __lu_com_camera_get_near_clip(lua_entity_id id);
    void __lu_com_camera_set_near_clip(lua_entity_id id, double near);
    double __lu_com_camera_get_far_clip(lua_entity_id id);
    void __lu_com_camera_set_far_clip(lua_entity_id id, double far);
]]

local Camera = {
    _entity_id = nil,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o._entity_id = entity_id
        C.__lu_com_camera_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return C.__lu_com_camera_exists(entity_id) end,
    remove = function(self) C.__lu_com_camera_remove(self._entity_id) end,

    getFov = function(self) return C.__lu_com_camera_get_fov(self._entity_id) end,
    setFov = function(self, fov) C.__lu_com_camera_set_fov(self._entity_id, fov) end,
    getNearClip = function(self) return C.__lu_com_camera_get_near_clip(self._entity_id) end,
    setNearClip = function(self, near) C.__lu_com_camera_set_near_clip(self._entity_id, near) end,
    getFarClip = function(self) return C.__lu_com_camera_get_far_clip(self._entity_id) end,
    setFarClip = function(self, far) C.__lu_com_camera_set_far_clip(self._entity_id, far) end
}

return Camera
