-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
-- entity Module - entity class for the entity-component-system and entity management.
-- @module entity
------------------------------------------------------------------------------

local ffi = require 'ffi'
local istype = ffi.istype
local C = ffi.C
local bit = require 'bit'
local band = bit.band
local bor = bit.bor
local bxor = bit.bxor
local bnot = bit.bnot
local lshift = bit.lshift

ffi.cdef[[
    bool __lu_entity_is_valid(lua_entity_id id);
    const char* __lu_entity_get_name(lua_entity_id id);
    void __lu_entity_set_name(lua_entity_id id, const char* name);
    uint32_t __lu_entity_get_flags(lua_entity_id id);
    void __lu_entity_set_flags(lua_entity_id id, uint32_t flags);
]]

--- entity flags for the entity class.
-- Keep in Sync with C++ com::entity_flags::$ in components.hpp
ENTITY_FLAGS = {
    NONE = 0, -- No flags
    HIDDEN = 0x1, -- entity is hidden in editor
    TRANSIENT = 0x2, -- entity is transient and will not be serialized
    STATIC = 0x4, -- entity is static and will not be moved
}

--- entity class
local entity = {
    id = 0
}

--- Creates a new entity from an entity id.
-- @tparam number id The valid entity id
function entity:fromId(id)
    if type(id) ~= 'number' then
        error('entity id is not a number alias lua_entity_id, but a '..type(id))
    end
    local o = {}
    setmetatable(o, {__index = self})   
    o.id = id
    return o
end

--- Checks if the entity is valid and alive.
-- @treturn bool True if the entity is valid and alive
function entity:isValid()
    return C.__lu_entity_is_valid(self.id)
end

--- Gets the name of the entity.
-- @treturn string The name of the entity
function entity:getName()
    return ffi.string(C.__lu_entity_get_name(self.id))
end

--- Sets the name of the entity.
-- @tparam string name The name of the entity
function entity:setName(name)
    C.__lu_entity_set_name(self.id, name)
end

--- Gets specified component from the entity or adds it if it does not exist.
-- @tparam components.Component component The component class
function entity:getComponent(component)
    return component:_new(self.id)
end

--- Checks if the entity has the specified component.
-- @tparam components.Component component The component class
function entity:hasComponent(component)
    return component._exists(self.id)
end

--- Gets the entity flags.
-- @treturn ENTITY_FLAGS The entity flags
function entity:getFlags()
    return C.__lu_entity_get_flags(self.id)
end

--- Sets the entity flags.
-- @tparam ENTITY_FLAGS flags The entity flags
function entity:setFlags(flags)
    C.__lu_entity_set_flags(self.id, flags)
end

--- Checks if the entity has the specified flags.
-- @tparam ENTITY_FLAGS flags The flags to check
function entity:hasFlag(flags)
    return band(self:getFlags(), flags) == flags
end

--- Adds the specified flags to the entity.
-- @tparam ENTITY_FLAGS flags The flags to add
function entity:addFlag(flags)
    if not self:isValid() then
        perror('entity id is invalid (0)')
        return
    end
    self:setFlags(bor(self:getFlags(), flags))
end

--- Removes the specified flags from the entity.
-- @tparam ENTITY_FLAGS flags The flags to remove
function entity:removeFlag(flags)
    self:setFlags(band(self:getFlags(), bnot(flags)))
end

--- Toggles the specified flags of the entity.
-- @tparam ENTITY_FLAGS flags The flags to toggle
function entity:toggleFlag(flags)
    self:setFlags(bxor(self:getFlags(), flags))
end

--- Checks if the entity is equal to another entity by comparing their ids.
-- @tparam entity other The other entity to compare
function entity:__eq(other)
    return self.id == other.id
end

--- Converts the entity to a string.
-- @treturn string The string representation of the entity ID
function entity:__tostring()
    return string.format('entity(%x)', self.id)
end

return entity
