-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Preload all files in this directory: TODO make this better
local module_cache = {
    ['app'] = 'lu/app',
    ['debugdraw'] = 'lu/debugdraw',
    ['entity'] = 'lu/entity',
    ['components'] = 'lu/components',
    ['input'] = 'lu/input',
    ['gmath'] = 'lu/gmath',
    ['quat'] = 'lu/quat',
    ['scene'] = 'lu/scene',
    ['time'] = 'lu/time',
    ['vec2'] = 'lu/vec2',
    ['vec3'] = 'lu/vec3',
    ['ini'] = 'lu/ini',
}

local loaded = {}

local function cache_module(key, module)
    print('Preloading module: '..key)
    assert(key ~= nil, 'Failed to preload module, missing key')
    assert(module ~= nil, 'Failed to preload module: '..key)
    loaded[key] = dofile('assets/scripts/'..module..'.lua')
    package.preload[key] = function() return loaded[key] end
end

for key, module in pairs(module_cache) do
    cache_module(key, module)
end

if engine_cfg.General.enableEditor then
    print('-- Editor is enabled --')
    cache_module('editor', 'editor/editor')
end

return loaded
