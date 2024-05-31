-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start() - Called once when the world is started
-- _update() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

local start_hook_id = '_start' -- hook id for start function
local tick_hook_id = '_update' -- hook id for tick function

-- Load all hook scripts in a directory
local function preload_all_hooks()
    local preload = require 'preload'
    assert(preload ~= nil and type(preload) == 'table')
    local hooks = {}
    for _, module in pairs(preload) do
        -- check if the script has a hook
        if module ~= nil and type(module) == 'table' and (module[start_hook_id] or module[tick_hook_id]) then
            table.insert(hooks, module)
        end
    end
    print('Loaded '..#hooks..' hooks')
    return hooks
end

local hook_mgr = {
    hooks = preload_all_hooks()
}
local hooks = hook_mgr.hooks

local scene = require 'scene'
assert(scene ~= nil)

local function execute_hooks()
    for i=1, #hooks do
        local hook = hooks[i]
        local routine = hook[tick_hook_id]
        if routine ~= nil then -- check if the hook has a tick function
            routine(hook) -- execute tick hook
        end
    end
end

function hook_mgr:tick()
    execute_hooks()
    scene._update()
end

return hook_mgr
