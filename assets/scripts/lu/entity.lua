-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    bool __lu_entity_is_valid(lua_entity_id id);
    bool __lu_entity_is_alive(lua_entity_id id);
    void __lu_entity_set_pos(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_entity_get_pos(lua_entity_id id);
    void __lu_entity_set_rot(lua_entity_id id, double x, double y, double z, double w);
    lua_vec4 __lu_entity_get_rot(lua_entity_id id);
    void __lu_entity_set_scale(lua_entity_id id, double x, double y, double z);
    lua_vec3 __lu_entity_get_scale(lua_entity_id id);
]]

local C = ffi.C
local istype = ffi.istype

local Entity = {id=0}

-- creates a new entity
function Entity:new(id)
    assert(type(id) == 'number')
    local o = {}
    setmetatable(o, {__index = self})   
    o.id = id
    return o
end

function Entity:isValid()
    return self.id ~= 0xffffffff and C.__lu_entity_is_valid(self.id)
end

function Entity:isAlive()
    return C.__lu_entity_is_alive(self.id)
end

function Entity:setPosition(pos)
    C.__lu_entity_set_pos(self.id, pos.x, pos.y, pos.z)
end

function Entity:getPosition()
    return C.__lu_entity_get_pos(self.id)
end

function Entity:setRotation(pos)
    C.__lu_entity_set_rot(self.id, pos.x, pos.y, pos.z, pos.w)
end

function Entity:getRotation()
    return C.__lu_entity_get_rot(self.id)
end

function Entity:setScale(pos)
    C.__lu_entity_set_scale(self.id, pos.x, pos.y, pos.z)
end

function Entity:getScale()
    return C.__lu_entity_get_scale(self.id)
end

return Entity
