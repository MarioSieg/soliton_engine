-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local istype = ffi.istype
local C = ffi.C

ffi.cdef[[
    bool __lu_entity_is_valid(lua_entity_id id);
    bool __lu_entity_is_alive(lua_entity_id id);
    const char* __lu_entity_get_name(lua_entity_id id);
    void __lu_entity_set_name(lua_entity_id id, const char* name);
]]

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
    return C.__lu_entity_is_valid(self.id) and C.__lu_entity_is_alive(self.id)
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

function Entity:__eq(other)
    return self.id == other.id
end

function Entity:__tostring()
    return string.format('Entity(%d)', self.id)
end

return Entity
