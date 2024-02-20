-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

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
    bool __lu_entity_is_alive(lua_entity_id id);
    const char* __lu_entity_get_name(lua_entity_id id);
    void __lu_entity_set_name(lua_entity_id id, const char* name);
    uint32_t __lu_entity_get_flags(lua_entity_id id);
    void __lu_entity_set_flags(lua_entity_id id, uint32_t flags);
]]

ENTITY_FLAGS = { -- Keep in Sync with com::entity_flags::$ in components.hpp
    NONE = 0,
    HIDDEN = 0x1,
    TRANSIENT = 0x2,
    STATIC = 0x4,
}

local Entity = {
    id = nil
}

-- creates a new entity
function Entity:fromId(id)
    assert(istype('lua_entity_id', id))
    local o = {}
    setmetatable(o, {__index = self})   
    o.id = id
    return o
end

function Entity:isValid()
    return self.id and C.__lu_entity_is_valid(self.id) and C.__lu_entity_is_alive(self.id)
end

function Entity:getName()
    return ffi.string(C.__lu_entity_get_name(self.id))
end

function Entity:setName(name)
    C.__lu_entity_set_name(self.id, name)
end

function Entity:component(component)
    return component:_new(self.id)
end

function Entity:hasComponent(component)
    return component._exists(self.id)
end

function Entity:getFlags()
    return C.__lu_entity_get_flags(self.id)
end

function Entity:setFlags(flags)
    C.__lu_entity_set_flags(self.id, flags)
end

function Entity:hasFlag(flags)
    return band(self:getFlags(), flags) == flags
end

function Entity:addFlag(flags)
    self:setFlags(bor(self:getFlags(), flags))
end

function Entity:removeFlag(flags)
    self:setFlags(band(self:getFlags(), bnot(flags)))
end

function Entity:toggleFlag(flags)
    self:setFlags(bxor(self:getFlags(), flags))
end

function Entity:__eq(other)
    return self.id == other.id
end

function Entity:__tostring()
    return string.format('Entity(%d)', self.id)
end

return Entity
