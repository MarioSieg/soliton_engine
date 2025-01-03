-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local quat = require 'quat'

local istype = ffi.istype -- Because vec2,3,4 types have special metatables, we need to serialize them manually.

local deserializer = {}

-- Copy fields from src to dst, if they are present in dst and src and of the same type.
function deserializer.copy_fields(dst, src)
    for k, source in pairs(src) do
        local val = dst[k] -- Cache value to avoid multiple lookups
        if not val then goto continue end -- Skip fields that are not present in dst

        if type(val) == type(source) then -- Check if the types are convertable
            if type(source) == 'table' then -- Recursively copy tables
                deserializer.copy_fields(val, source)
            elseif type(source) == 'number' or type(source) == 'string' or type(source) == 'boolean' then -- Directly copy these types
                dst[k] = source
            end
        -- Vectors are special cdata in dst, not tables, so we need to construct new instances of vec2,3,4
        elseif istype('__vec2', val) then -- Construct proper vec2 instance
            dst[k] = vec2(source.x, source.y)
        elseif istype('__vec3', val) then -- Construct proper vec3 instance
            dst[k] = vec3(source.x, source.y, source.z)
        elseif istype('__vec4', val) then -- Construct proper vec4 instance
            dst[k] = quat(source.x, source.y, source.z, source.w)
        else
            eprint(string.format('Deserialize: Field "%s" of type "%s" is not present in dst or is of a different type.', k, type(k)))
        end
        ::continue::
    end
end

return deserializer
