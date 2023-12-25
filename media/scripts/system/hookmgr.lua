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

-- Load all hook scripts in a directory
local function load_hooks_recursive(search_dir)
    print('Loading hook scripts in: '..search_dir)
    local on_start_hooks = {}
    local on_tick_hooks = {}
    for entry in lfs.dir(search_dir) do
        if entry ~= '.' and entry ~= '..' then -- only load .lua files
            local full_path = search_dir..'/'..entry
            local attribs = lfs.attributes(full_path).mode
            if attribs == 'file' and entry:sub(-#'.lua') == '.lua' then -- only load files
                local on_start, on_tick = load_hook_script(full_path)
                if on_start ~= nil then -- add the hook to the global start hook list
                    table.insert(on_start_hooks, on_start)
                end
                if on_tick ~= nil then -- add the hook to the global tick hook list
                    table.insert(on_tick_hooks, on_tick)
                end
            elseif attribs == 'directory' then -- load all scripts in subdirectories
                local start_hooks, tick_hooks = load_hooks_recursive(full_path)
                for _, hook in ipairs(start_hooks) do
                    table.insert(on_start_hooks, hook)
                end
                for _, hook in ipairs(tick_hooks) do
                    table.insert(on_tick_hooks, hook)
                end
            end
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
