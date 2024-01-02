-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

-- Preload all files in this directory:
local MODULES = {
    ['App'] = 'lu/app',
    ['Color'] = 'lu/color',
    ['Debug'] = 'lu/debug',
    ['Math'] = 'lu/math',
    ['Quat'] = 'lu/quat',
    ['Scene'] = 'lu/scene',
    ['Time'] = 'lu/time',
    ['Vec2'] = 'lu/vec2',
    ['Vec3'] = 'lu/vec3',
}

local loaded = {}

local function loadMod(key, module)
    print('Preloading module: '..key)
    assert(key ~= nil, 'Failed to preload module, missing key')
    assert(module ~= nil, 'Failed to preload module: '..key)
    loaded[key] = dofile('media/scripts/'..module..'.lua')
    package.preload[key] = function() return loaded[key] end
end

for key, module in pairs(MODULES) do
    loadMod(key, module)
end

if SYSTEM_CFG.editor_enable then
    loadMod('Editor', 'editor/editor')
end

return loaded
