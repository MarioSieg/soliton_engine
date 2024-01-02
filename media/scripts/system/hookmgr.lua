-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start_() - Called once when the world is started
-- _tick_() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

local function searchHookTargetsFromDirRecursive(searchDir)
    assert(searchDir ~= nil and type(searchDir) == 'string')
    local targets = {}
    for entry in lfs.dir(searchDir) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = searchDir..'/'..entry
            local attribs = lfs.attributes(fullPath).mode
            if attribs == 'file' and entry:sub(-#'.lua') == '.lua' then -- load files
               table.insert(targets, fullPath)
            elseif attribs == 'directory' then -- add dirs
                for _, target in ipairs(searchHookTargetsFromDirRecursive(fullPath)) do
                    table.insert(targets, target)
                end
            end
        end
    end

    -- sort tables based on directory tree depth, so that scripts in subdirectories are loaded last
    table.sort(targets, function(a, b)
        local aDepth = 0
        local bDepth = 0
        for _ in a:gmatch('/') do aDepth = aDepth + 1 end
        for _ in b:gmatch('/') do bDepth = bDepth + 1 end
        return aDepth < bDepth
    end)
    assert(targets ~= nil)
    print('Found '..#targets..' hook scripts in: '..searchDir)
    return targets
end

local ON_START_HOOK = '__onStart'
local ON_TICK_HOOK = '__onTick'

-- Load all hook scripts in a directory
local function preloadHooks()
    local preload = require 'preload'
    assert(preload ~= nil and type(preload) == 'table')
    local hooks = {}
    for key, module in pairs(preload) do
        -- check if the script has a hook
        if module ~= nil and type(module) == 'table' and (module[ON_START_HOOK] or module[ON_TICK_HOOK]) then
            table.insert(hooks, module)
        end
    end
    print('Loaded '..#hooks..' hooks')
    return hooks
end

local HookManager = {
    HOOK_DIR = 'media/scripts/lu',
    hooks = preloadHooks()
}

local Scene = require ('Scene')

local function tickEngineSystems()
    for _, hook in ipairs(HookManager.hooks) do
        local routine = hook[ON_TICK_HOOK]
        if routine ~= nil then -- check if the hook has a tick function
            routine(hook) -- execute tick hook
        end
    end
end

local function tickScene()
    if Scene.active then
        Scene.active:__onTick()
    end
end

function HookManager:tick()
    tickEngineSystems()
    tickScene()
end

return HookManager
