-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local C = ffi.C

ffi.cdef[[
    bool __lu_com_character_controller_exists(lua_entity_id id);
    void __lu_com_character_controller_add(lua_entity_id id);
    void __lu_com_character_controller_remove(lua_entity_id id);
    lua_vec3 __lu_com_character_controller_get_linear_velocity(lua_entity_id id);
    void __lu_com_character_controller_set_linear_velocity(lua_entity_id id, lua_vec3 velocity);
    int __lu_com_character_controller_get_ground_state(lua_entity_id id);
    lua_vec3 __lu_com_character_controller_get_ground_normal(lua_entity_id id);
]]

CHARACTER_GROUND_STATE = { -- Keep in sync with C++: CharacterBase.h 
    ON_GROUND = 0, -- Character is on the ground and can move freely.
    ON_STEEP_GROUND = 1, -- Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
    NOT_SUPPORTED = 2, -- Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
    IN_AIR = 3 -- Character is in the air and is not touching anything.
}

local CharacterController = {
    _entity_id = nil,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, {__index = self})
        o._entity_id = entity_id
        C.__lu_com_character_controller_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return C.__lu_com_character_controller_exists(entity_id) end,
    remove = function(self) C.__lu_com_character_controller_remove(self._entity_id) end,

    getLinearVelocity = function(self) return C.__lu_com_character_controller_get_linear_velocity(self._entity_id) end,
    setLinearVelocity = function(self, velocity) C.__lu_com_character_controller_set_linear_velocity(self._entity_id, velocity) end,
    getGroundState = function(self) return C.__lu_com_character_controller_get_ground_state(self._entity_id) end,
    getGroundNormal = function(self) return C.__lu_com_character_controller_get_ground_normal(self._entity_id) end
}

return CharacterController
