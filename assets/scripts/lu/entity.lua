-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local istype = ffi.istype
local C = ffi.C

ffi.cdef[[
    bool __lu_entity_is_valid(lua_entity_id id);
    bool __lu_entity_is_alive(lua_entity_id id);
]]

local Entity = {
    id = nil
}

-- creates a new entity
function Entity:new(id)
    assert(istype('lua_entity_id', id))
    local o = {}
    setmetatable(o, {__index = self})   
    o.id = id
    return o
end

function Entity:isValid()
    return C.__lu_entity_is_valid(self.id) and C.__lu_entity_is_alive(self.id)
end

function Entity:component(component)
    return component:new(self.id)
end

return Entity
