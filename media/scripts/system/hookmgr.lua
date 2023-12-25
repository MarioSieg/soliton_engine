-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start_() - Called once when the world is started
-- _tick_() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

-- Load and execute a single hook script
local function load_hook_script(file)
    print('Executing hook script: '..file)
    local chunk, err = loadfile(file) -- load the script
    if chunk == nil then
        panic('Failed to load script: '..file..': '..err)
    end
    local mod = chunk() -- execute the script
    return mod._on_start_, mod._on_tick_ -- return the two hooks
end

local function search_hook_targets(search_dir)
    assert(search_dir ~= nil and type(search_dir) == 'string')
    local targets = {}
    for entry in lfs.dir(search_dir) do
        if entry ~= '.' and entry ~= '..' then
            local full_path = search_dir..'/'..entry
            local attribs = lfs.attributes(full_path).mode
            if attribs == 'file' and entry:sub(-#'.lua') == '.lua' then -- load files
               table.insert(targets, full_path)
            elseif attribs == 'directory' then -- add dirs
                for _, target in ipairs(search_hook_targets(full_path)) do
                    table.insert(targets, target)
                end
            end
        end
    end
    -- sort tables based on directory tree depth, so that scripts in subdirectories are loaded last
    table.sort(targets, function(a, b)
        local a_depth = 0
        local b_depth = 0
        for _ in a:gmatch('/') do a_depth = a_depth + 1 end
        for _ in b:gmatch('/') do b_depth = b_depth + 1 end
        return a_depth < b_depth
    end)
    assert(targets ~= nil)
    for _, target in ipairs(targets) do
        print('Found hook script: '..target)
    end
    return targets
end

-- Load all hook scripts in a directory
local function load_hooks_recursive(search_dir)
    assert(search_dir ~= nil and type(search_dir) == 'string')
    print('Loading hook scripts in: '..search_dir)
    local on_start_hooks = {}
    local on_tick_hooks = {}
    for _, target in ipairs(search_hook_targets(search_dir)) do
        local on_start, on_tick = load_hook_script(target)
        if on_start then
            table.insert(on_start_hooks, on_start)
        end
        if on_tick then
            table.insert(on_tick_hooks, on_tick)
        end
    end
    return on_start_hooks, on_tick_hooks
end
local M = {
    HOOK_DIR = 'media/scripts/lu',
    on_start_hooks = {}, -- TODO: Invoke these hooks
    on_tick_hooks = {}
}

local start_hooks, tick_hooks = load_hooks_recursive(M.HOOK_DIR)
M.on_start_hooks = start_hooks
M.on_tick_hooks = tick_hooks
print('Loaded '..#M.on_start_hooks..' start hooks')
print('Loaded '..#M.on_tick_hooks..' tick hooks')

function M:tick()
    for _, hook in ipairs(self.on_tick_hooks) do
        hook() -- execute all tick hooks
    end
end

return M
