-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start() - Called once when the world is started
-- _update() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

local start_hook_id = '_start' -- hook id for start function
local tick_hook_id = '_update' -- hook id for tick function

-- Load all hook from all currently loaded module
local function preload_all_hooks()
    local hooks = {}
    for key, module in pairs(package.loaded) do
        -- check if the script has a hook
        if module ~= nil and type(module) == 'table' then
            pcall(function()
                if (module[start_hook_id] or module[tick_hook_id]) then -- check if the module has a start or tick function
                    print('Found hook in module ' .. key)
                    table.insert(hooks, module)
                end
            end)
        end
    end
    print('Loaded ' .. #hooks .. ' hooks')
    return hooks
end

local hook_mgr = {
    hooks = preload_all_hooks()
}
local hooks = hook_mgr.hooks

local function tick_all_hooks()
    for i = 1, #hooks do
        local hook = hooks[i]
        local routine = hook[tick_hook_id]
        if routine ~= nil then -- check if the hook has a tick function
            routine(hook) -- execute tick hook
        end
    end
end

function hook_mgr:tick()
    tick_all_hooks()
end

return hook_mgr
