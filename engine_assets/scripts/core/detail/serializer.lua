-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

-- The serializer serializes objects into a table, which can then be serialized to JSON or other formats.
-- It only serializes public fields of an object, which are fields that do not start with an underscore.
-- It also serializes vec2, vec3, and vec4 types into tables with x, y, z, and w fields.
-- Tables can implement the _serialize method to customize their serialization.

local ffi = require 'ffi'
local istype = ffi.istype -- Because vec2,3,4 types have special metatables, we need to serialize them manually.

local serializer = {}

local function is_public_field(k)
    return k:byte(1) ~= 0x5f -- 0x5f is the ASCII code for '_'
end

function serializer.serialize(object)
    if type(object) == 'number' or type(object) == 'string' or type(object) == 'boolean' then -- Directly serialize these types
        return object
    elseif istype('__vec2', object) then -- Serialize vec2
        return { x = object.x, y = object.y }
    elseif istype('__vec3', object) then -- Serialize vec3
        return { x = object.x, y = object.y, z = object.z }
    elseif istype('__vec4', object) then -- Serialize vec4
        return { x = object.x, y = object.y, z = object.z, w = object.w }
    elseif type(object) == 'table' then -- Serialize tables recursively
        if object._serialize then -- If the table has a custom serialization method, use it
            return object:_serialize()
        else -- Serialize the table recursively
            local fields = {}
            for k, v in pairs(object) do -- Serialize all fields
                if is_public_field(k) then -- We are only interested in public fields
                    fields[k] = serializer.serialize(v)
                end
            end
            return fields
        end
    else
        return nil
    end
end

return serializer
