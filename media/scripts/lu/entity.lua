-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    typedef struct {
        double x;
        double y;
        double z;
    } Vec3;
    bool __lu_entity_is_valid(double id);
    bool __lu_entity_is_alive(double id);
    void __lu_entity_set_pos(double id, double x, double y, double z);
    Vec3 __lu_entity_get_pos(double id);
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
    assert(istype('Vec3', pos))
    C.__lu_entity_set_pos(id, pos.x, pos.y, pos.z)
end

function Entity.getPos(id)
    assert(type(id) == 'number')
    return C.__lu_entity_get_pos(id)
end

return Entity
