-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start_() - Called once when the world is started
-- _tick_() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

-- Load and execute a single hook script
local function loadHookScriptInstance(file)
    local chunk, err = loadfile(file) -- load the script
    if chunk == nil then
        panic('Failed to load script: '..file..': '..err)
    end
    return chunk() -- execute the script
end

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
local MANUAL_HOOK_FILES = {}

if SYSTEM_CFG.editor_enable then -- load editor hook
    table.insert(MANUAL_HOOK_FILES, 'media/scripts/editor/editor.lua')
end

-- Load all hook scripts in a directory
local function initCoreHooks(searchDir)
    assert(searchDir ~= nil and type(searchDir) == 'string')
    print('Loading hook scripts in: '..searchDir)
    local candicates = searchHookTargetsFromDirRecursive(searchDir)
    for _, file in ipairs(MANUAL_HOOK_FILES) do
        table.insert(candicates, file)
    end
    local hooks = {}
    for _, file in ipairs(candicates) do
        local target = loadHookScriptInstance(file)
        if target ~= nil and type(target) == 'table' and (target[ON_START_HOOK] or target[ON_TICK_HOOK]) then -- check if the script has a hook
            table.insert(hooks, target)
        end
    end
    return hooks
end

local m = {
    HOOK_DIR = 'media/scripts/lu',
    hooks = {}
}

m.hooks = initCoreHooks(m.HOOK_DIR)
print('Loaded '..#m.hooks..' hooks')

if not scene then -- check if the scene module is loaded
    panic('scene is not defined')
end

local function tickEngineSystems()
    for _, hook in ipairs(m.hooks) do
        local routine = hook[ON_TICK_HOOK]
        if routine ~= nil then -- check if the hook has a tick function
            routine(hook) -- execute tick hook
        end
    end
end

local function tickScene()
    if scene.active then
        scene.active:__onTick()
    end

end

function m:tick()
    tickEngineSystems()
    tickScene()
end

return m
