--- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

----------------------------------------------------------------------------
--- entity Module - entity class for the entity-component-system and entity management.
--- @module entity
------------------------------------------------------------------------------

local ffi = require 'ffi'
local bit = require 'bit'
local band, bor, bxor, bnot = bit.band, bit.bor, bit.bxor, bit.bnot
local cpp = ffi.C

ffi.cdef [[
    bool __lu_entity_is_valid(lua_entity_id id);
    const char* __lu_entity_get_name(lua_entity_id id);
    void __lu_entity_set_name(lua_entity_id id, const char* name);
    uint32_t __lu_entity_get_flags(lua_entity_id id);
    void __lu_entity_set_flags(lua_entity_id id, uint32_t flags);

    void __lu_scene_despawn_entity(lua_entity_id id);
]]

--- entity flags for the entity class.
--- Keep in Sync with cpp++ com::entity_flags::$ in components.hpp
entity_flags = {
    none = 0, --- No flags
    hidden = 0x1, --- entity is hidden in editor
    transient = 0x2, --- entity is transient and will not be serialized
    static = 0x4, --- entity is static and will not be moved
}

--- entity class
local entity = {
    _id = 0,
    _component_cache = {}
}

--- Creates a new entity from an entity id.
--- @tparam number id The valid entity id
function entity:from_native_id(id)
    local o = {}
    setmetatable(o, { __index = self })
    o._id = id
    o._component_cache = {}
    return o
end

--- Checks if the entity is valid and alive.
--- @treturn bool True if the entity is valid and alive
function entity:is_valid()
    return cpp.__lu_entity_is_valid(self._id)
end

--- Gets the name of the entity.
--- @treturn string The name of the entity
function entity:get_name()
    return ffi.string(cpp.__lu_entity_get_name(self._id))
end

--- Sets the name of the entity.
--- @tparam string name The name of the entity
function entity:set_name(name)
    cpp.__lu_entity_set_name(self._id, name)
end

--- Gets specified component from the entity or adds it if it does not exist.
--- @tparam components.Component component The component class
function entity:get_component(component)
    assert(component ~= nil)
    local com_id = component._id
    if self._component_cache[com_id] then
        return self._component_cache[com_id]
    else
        local instance = component:_new(self._id)
        self._component_cache[com_id] = instance
        return instance
    end
end

--- Removes the specified component from the entity.
--- @tparam components.Component component The component class
function entity:remove_component(component)
    assert(component ~= nil)
    local com_id = component._id
    if self:has_component(component) then
        component._remove(self._id)
        self._component_cache[com_id] = nil
    end
end

--- Checks if the entity has the specified component.
--- @tparam components.Component component The component class
function entity:has_component(component)
    assert(component ~= nil)
    local com_id = component._id
    return self._component_cache[com_id] ~= nil or component._exists(self._id)
end

--- Gets the entity flags.
--- @treturn ENTITY_FLAGS The entity flags
function entity:get_flags()
    return cpp.__lu_entity_get_flags(self._id)
end

--- Sets the entity flags.
--- @tparam ENTITY_FLAGS flags The entity flags
function entity:set_flags(flags)
    cpp.__lu_entity_set_flags(self._id, flags)
end

--- Checks if the entity has the specified flags.
--- @tparam ENTITY_FLAGS flags The flags to check
function entity:has_flag(flags)
    return band(self:get_flags(), flags) ~= 0
end

--- Adds the specified flags to the entity.
--- @tparam ENTITY_FLAGS flags The flags to add
function entity:add_flag(flags)
    self:set_flags(bor(self:get_flags(), flags))
end

--- Removes the specified flags from the entity.
--- @tparam ENTITY_FLAGS flags The flags to remove
function entity:remove_flag(flags)
    self:set_flags(band(self:get_flags(), bnot(flags)))
end

--- Toggles the specified flags of the entity.
--- @tparam ENTITY_FLAGS flags The flags to toggle
function entity:toggle_flag(flags)
    self:set_flags(bxor(self:get_flags(), flags))
end

--- Despawns the entity. Same as scene.despawn(entity)
--- @see scene.despawn
function entity:despawn()
    cpp.__lu_scene_despawn_entity(self._id)
end

--- Checks if the entity is equal to another entity by comparing their ids.
--- @tparam entity other The other entity to compare
function entity:__eq(other)
    return self._id == other._id
end

--- Converts the entity to a string.
--- @treturn string The string representation of the entity ID
function entity:__tostring()
    return string.format('entity(%x)', self._id)
end

return entity
