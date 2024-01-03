-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    bool __lu_entity_is_valid(double id);
    bool __lu_entity_is_alive(double id);
]]

local C = ffi.C

local Entity = {}

function Entity.isValid(id)
    assert(type(id) == 'number', 'id must be a number')
    return C.__lu_entity_is_valid(id)
end

function Entity.isAlive(id)
    assert(type(id) == 'number', 'id must be a number')
    return C.__lu_entity_is_alive(id)
end

return Entity
