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

local Entity = {}

function Entity.isValid(id)
    assert(type(id) == 'number')
    return C.__lu_entity_is_valid(id)
end

function Entity.isAlive(id)
    assert(type(id) == 'number')
    return C.__lu_entity_is_alive(id)
end

function Entity.setPos(id, pos)
    assert(type(id) == 'number')
    assert(istype('lua_vec3', pos))
    C.__lu_entity_set_pos(id, pos.x, pos.y, pos.z)
end

function Entity.getPos(id)
    assert(type(id) == 'number')
    return C.__lu_entity_get_pos(id)
end

function Entity.setRot(id, pos)
    assert(type(id) == 'number')
    assert(istype('lua_vec4', pos))
    C.__lu_entity_set_rot(id, pos.x, pos.y, pos.z, pos.w)
end

function Entity.getRot(id)
    assert(type(id) == 'number')
    return C.__lu_entity_get_rot(id)
end

function Entity.setScale(id, pos)
    assert(type(id) == 'number')
    assert(istype('lua_vec3', pos))
    C.__lu_entity_set_scale(id, pos.x, pos.y, pos.z)
end

function Entity.getScale(id)
    assert(type(id) == 'number')
    return C.__lu_entity_get_scale(id)
end

return Entity
