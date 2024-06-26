-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local function cache_module(key, module)
    print('Preloading module: ' .. key)
    assert(key ~= nil, 'Failed to preload module, missing key')
    assert(module ~= nil, 'Failed to preload module: '..key)
    return require('assets/scripts/' .. module)
end

if engine_cfg.General.enableEditor then
    print('-- Editor is enabled --')
    cache_module('editor', 'editor/editor')
end

local loaded = {}
for _, module in pairs(package.loaded) do
    table.insert(loaded, module)
end

return loaded
