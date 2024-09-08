-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- camera Module - Implements the camera component.
--- @module camera
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
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

local camera = {
    _id = 0x765be778,
    _entity_id = 0,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, { __index = self })
        o._entity_id = entity_id
        cpp.__lu_com_camera_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return cpp.__lu_com_camera_exists(entity_id) end,
    _remove = function(entity_id) cpp.__lu_com_camera_remove(entity_id) end,

    get_fov = function(self) return cpp.__lu_com_camera_get_fov(self._entity_id) end,
    set_fov = function(self, fov) cpp.__lu_com_camera_set_fov(self._entity_id, fov) end,
    get_near_clip = function(self) return cpp.__lu_com_camera_get_near_clip(self._entity_id) end,
    set_near_clip = function(self, near) cpp.__lu_com_camera_set_near_clip(self._entity_id, near) end,
    get_far_clip = function(self) return cpp.__lu_com_camera_get_far_clip(self._entity_id) end,
    set_far_clip = function(self, far) cpp.__lu_com_camera_set_far_clip(self._entity_id, far) end,
    get_matrices = function(self, view, proj) cpp.__lu_com_camera_get_matrices(view, proj) end
}

return camera
