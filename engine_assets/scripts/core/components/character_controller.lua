-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- character_controller Module - Implements the character-controller component.
--- @module character_controller
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C

ffi.cdef [[
    bool __lu_com_character_controller_exists(lua_entity_id id);
    void __lu_com_character_controller_add(lua_entity_id id);
    void __lu_com_character_controller_remove(lua_entity_id id);
    lua_vec3 __lu_com_character_controller_get_linear_velocity(lua_entity_id id);
    void __lu_com_character_controller_set_linear_velocity(lua_entity_id id, lua_vec3 velocity);
    int __lu_com_character_controller_get_ground_state(lua_entity_id id);
    lua_vec3 __lu_com_character_controller_get_ground_normal(lua_entity_id id);
]]

ground_state = { -- Keep in sync with cpp++: CharacterBase.h
    on_ground = 0, -- Character is on the ground and can move freely.
    on_steep_ground = 1, -- Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
    not_supported = 2, -- Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
    flying = 3 -- Character is in the air and is not touching anything.
}

local character_controller = {
    _id = 0x598647ca,
    _entity_id = 0,
    _new = function(self, entity_id)
        local o = {}
        setmetatable(o, { __index = self })
        o._entity_id = entity_id
        cpp.__lu_com_character_controller_add(entity_id)
        return o
    end,
    _exists = function(entity_id) return cpp.__lu_com_character_controller_exists(entity_id) end,
    _remove = function(entity_id) cpp.__lu_com_character_controller_remove(entity_id) end,
    _serialize = function(self)
        return {}
    end,

    get_linear_velocity = function(self) return cpp.__lu_com_character_controller_get_linear_velocity(self._entity_id) end,
    set_linear_velocity = function(self, velocity) cpp.__lu_com_character_controller_set_linear_velocity(self._entity_id, velocity) end,
    get_ground_state = function(self) return cpp.__lu_com_character_controller_get_ground_state(self._entity_id) end,
    get_ground_normal = function(self) return cpp.__lu_com_character_controller_get_ground_normal(self._entity_id) end
}

return character_controller
