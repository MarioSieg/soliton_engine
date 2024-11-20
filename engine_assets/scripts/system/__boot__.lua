-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file is very important, it's the first script that gets executed by the engine.
-- It's responsible for loading all other scripts and setting up the environment.
-- It also contains the main tick loop.
-- Only modify this file if you know what you're doing.

local jit = require 'jit'
local ffi = require 'ffi'
local cpp = ffi.C
local max = math.max

lua_include_dirs = {
    'engine_assets/scripts',
    'engine_assets/scripts/core',
    'engine_assets/scripts/lib'
}

jit.off()
for _, path in ipairs(lua_include_dirs) do
    package.path = string.format('%s;%s/?.lua', package.path, path)
end

require 'system.cdefs' -- Load the C definitions
if cpp.__lu_ffi_cookie() ~= 0xfefec0c0 then
    panic('Invalid FFI cookie!')
end
function panic(msg)
    cpp.__lu_panic(msg)
end

require 'system.extensions'
require 'config.engine'
if engine_cfg['system']['enable_jit'] == true then
    jit.on() -- Enable JIT
    jit.opt.start('+fma') -- enable FMA for better performance
    if engine_cfg['system']['enable_jit_disasm'] then
        require('jit.dump').on('m', 'jit.log')
    end
else
    print('! JIT is disabled')
end
print(string.format('%s %s %s', jit.version, jit.os, jit.arch))
print('JIT active ->')
print(jit.status())
print('GC64: ' .. (ffi.abi('gc64') and 'yes' or 'no'))

-- ###################### SYSTEM BOOTSTRAP ######################

local start_hook_id = '_start'
local tick_hook_id = '_update'
local function search_hooks()
    local hooks = {}
    for key, module in pairs(package.loaded) do
        if module ~= nil and type(module) == 'table' then
            pcall(function()
                local has_hooks = false
                if module[start_hook_id] then
                    print('Found start hook in module ' .. key)
                    has_hooks = true
                end
                if module[tick_hook_id] then
                    print('Found tick hook in module ' .. key)
                    has_hooks = true
                end
                if has_hooks then
                    table.insert(hooks, module)
                end
            end)
        end
    end
    print('Loaded ' .. #hooks .. ' hooks')
    return hooks
end
local function invoke_hooks(hooks, id)
    for i = 1, #hooks do
        local hook = hooks[i]
        local routine = hook[id]
        if routine ~= nil then -- check if the hook has a start function
            routine(hook) -- execute start hook
        end
    end
end
require 'system.redirect' -- Load the redirect script
local hooks = search_hooks()
function __on_start__()
    invoke_hooks(hooks, start_hook_id)
    collectgarbage_full_cycle()
end

local clock = os.clock
local gc_clock = clock()
local cfg = engine_cfg['system']

function __on_tick__()
    invoke_hooks(hooks, tick_hook_id)
    if cfg['auto_gc_time_stepping'] then
        local diff = clock() - gc_clock
        local target = 1.0 / max(cfg['target_fps'], 1.0)
        local delta = target - diff
        local lim = cfg['auto_gc_time_stepping_step_limit']
        if delta > lim then
            local done = false
            -- -- until garbage is all collected or further collection would exceed the threshold
            while not done and target - diff > lim do
                done = collectgarbage('step', 1)
                diff = clock() - gc_clock
            end
        end
    else
        collectgarbage('step') -- manually execute one GC step every frame
    end
    collectgarbage('stop') -- stop the GC because on step resumes it -> we run it manually every frame
end
