-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
-- JSON serializaer/deserializer

local decode = require 'ext.lunajson.decoder'()
local encode = require 'ext.lunajson.encoder'()

local function serialize_to_file(path, data)
    local file = io.open(path, 'w')
    file:write(encode(data))
    file:close()
end

local function deserialize_from_file(path)
    local file = io.open(path, 'r')
    local data = decode(file:read('*a'))
    file:close()
    return data
end

return {
    serialize = encode,
    deserialize = decode,
    serialize_to_file = serialize_to_file,
    deserialize_from_file = deserialize_from_file,
}
