-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
-- JSON serializaer/deserializer

local decoder = (require('lunajson.decoder'))()
local encoder = (require('lunajson.encoder'))()

local function save(path, data)
    local file = io.open(path, 'w')
    file:write(encoder(data))
    file:close()
end

local function load(path)
    local file = io.open(path, 'r')
    local data = decoder(file:read('*a'))
    file:close()
    return data
end

return {
    decode = decoder,
    encode = encoder,
    save = save,
    load = load
}
