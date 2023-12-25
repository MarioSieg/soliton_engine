-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Engine hook script loading and management

-- Script in Lunam have two hooks:
-- _start_() - Called once when the world is started
-- _tick_() - Called every frame
-- Other loading can be done on global scope when the script is loaded.

-- Load and execute a single hook script
local function load_hook_script(file)
    local chunk, err = loadfile(file) -- load the script
    if chunk == nil then
        panic('Failed to load script: '..file..': '..err)
    end
    return chunk() -- execute the script
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
    return targets
end

-- Load all hook scripts in a directory
local function init_hooks(search_dir)
    assert(search_dir ~= nil and type(search_dir) == 'string')
    print('Loading hook scripts in: '..search_dir)
    local hooks = {}
    for _, target in ipairs(search_hook_targets(search_dir)) do
        local target = load_hook_script(target)
        assert(target ~= nil and type(target) == 'table')
        if target['_on_start_'] or target['_on_tick_'] then -- check if the script has a hook
            table.insert(hooks, target)
        end
    end
    return hooks
end
local M = {
    HOOK_DIR = 'media/scripts/lu',
    hooks = {}
}

M.hooks = init_hooks(M.HOOK_DIR)
print('Loaded '..#M.hooks..' hooks')

function M:tick()
    for _, hook in ipairs(self.hooks) do
        if hook['_on_tick_'] ~= nil then -- check if the hook has a tick function
            hook:_on_tick_() -- execute tick hook
        end
    end
end

return M
