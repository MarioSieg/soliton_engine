-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Preload all files in this directory: TODO make this better
local module_cache = {
    ['app'] = 'lu/app',
    ['color'] = 'lu/color',
    ['debug'] = 'lu/debug',
    ['entity'] = 'lu/entity',
    ['components'] = 'lu/components',
    ['input'] = 'lu/input',
    ['math'] = 'lu/math',
    ['quat'] = 'lu/quat',
    ['scene'] = 'lu/scene',
    ['time'] = 'lu/time',
    ['tween'] = 'lu/tween',
    ['vec2'] = 'lu/vec2',
    ['vec3'] = 'lu/vec3',
    ['ini'] = 'lu/ini',
}

local loaded = {}

local function loadMod(key, module)
    print('Preloading module: '..key)
    assert(key ~= nil, 'Failed to preload module, missing key')
    assert(module ~= nil, 'Failed to preload module: '..key)
    loaded[key] = dofile('assets/scripts/'..module..'.lua')
    package.preload[key] = function() return loaded[key] end
end

for key, module in pairs(module_cache) do
    loadMod(key, module)
end

if ENGINE_CONFIG.General.enableEditor then
    loadMod('Editor', 'editor/editor')
end

return loaded
