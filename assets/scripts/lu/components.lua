-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local istype = ffi.istype
local C = ffi.C

local Components = {}

-- ################## TRANSFORM ##################
ffi.cdef[[
    void __lu_com_transform_set_pos(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_com_transform_get_pos(lua_entity_id id);
    void __lu_com_transform_set_rot(lua_entity_id id, double x, double y, double z, double w);
    lua_vec4 __lu_com_transform_get_rot(lua_entity_id id);
    void __lu_com_transform_set_scale(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_com_transform_get_scale(lua_entity_id id);
]]
Components.TRANSFORM = {
    entity_id = nil,
    new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o.entity_id = entity_id
        return o
    end,
    setPosition = function(self, pos) C.__lu_com_transform_set_pos(self.entity_id, pos.x, pos.y, pos.z) end,
    getPosition = function(self) return C.__lu_com_transform_get_pos(self.entity_id) end,
    setRotation = function(self, rot) C.__lu_com_transform_set_rot(self.entity_id, rot.x, rot.y, rot.z, rot.w) end,
    getRotation = function(self) return C.__lu_com_transform_get_rot(self.entity_id) end,
    setScale = function(self, scale) C.__lu_com_transform_set_scale(self.entity_id, scale.x, scale.y, scale.z) end,
    getScale = function(self) return C.__lu_com_transform_get_scale(self.entity_id) end
}

-- ################## CAMERA ##################
ffi.cdef[[
    void __lu_com_camera_add(lua_entity_id id);
    void __lu_com_camera_remove(lua_entity_id id);
    double __lu_com_camera_get_fov(lua_entity_id id);
    void __lu_com_camera_set_fov(lua_entity_id id, double fov);
    double __lu_com_camera_get_near_clip(lua_entity_id id);
    void __lu_com_camera_set_near_clip(lua_entity_id id, double near);
    double __lu_com_camera_get_far_clip(lua_entity_id id);
    void __lu_com_camera_set_far_clip(lua_entity_id id, double far);
]]
Components.CAMERA = {
    entity_id = nil,
    new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o.entity_id = entity_id
        C.__lu_com_camera_add(entity_id)
        return o
    end,
    remove = function(self) C.__lu_com_camera_remove(self.entity_id) end,
    getFov = function(self) return C.__lu_com_camera_get_fov(self.entity_id) end,
    setFov = function(self, fov) C.__lu_com_camera_set_fov(self.entity_id, fov) end,
    getNearClip = function(self) return C.__lu_com_camera_get_near_clip(self.entity_id) end,
    setNearClip = function(self, near) C.__lu_com_camera_set_near_clip(self.entity_id, near) end,
    getFarClip = function(self) return C.__lu_com_camera_get_far_clip(self.entity_id) end,
    setFarClip = function(self, far) C.__lu_com_camera_set_far_clip(self.entity_id, far) end
}

return Components
