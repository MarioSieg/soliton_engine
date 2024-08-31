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

-- Dont't waste traces, machine-code and other memory jitting any boot code, it's only run once but stays in memory forever.
-- We turn the JIT back on before entering the main tick loop below.
jit.off()

-- We load panic manually because it's not in the package path yet
ffi.cdef [[
    void __lu_panic(const char* msg);
    uint32_t __lu_ffi_cookie(void);
]]

function panic(msg)
    cpp.__lu_panic(msg)
end

-- Check that FFI calls work
local cookie = cpp.__lu_ffi_cookie()
if cookie ~= 0xfefec0c0 then
    panic('Invalid FFI cookie!')
end

-- add all other directories to package paths
for _, path in ipairs(lua_include_dirs) do
    package.path = string.format('%s;%s/?.lua', package.path, path)
end

-- ###################### SYSTEM BOOTSTRAP ######################
-- Everything is now set up to load the engine context

local engine_ctx = require 'system.setup' -- Lazy load the setup script
assert(engine_ctx ~= nil) -- Ensure the setup script was loaded
assert(engine_cfg ~= nil) -- Ensure the engine config was loaded

-- DO NOT Rename - Invoked from native code
-- This function is called before the main tick loop starts.
-- It's responsible for loading all tick hooks of global engine components.
-- Note: Scripts which are loaded per-world are loaded in the world-setup script.
function __on_prepare__()
    engine_ctx:inject_hooks() -- Load all start/tick hooks

    if engine_cfg['system']['enable_jit'] == true then
        jit.on() -- Enable JIT
        jit.opt.start('+fma') -- enable FMA for better performance
        if engine_cfg['system']['enable_jit_disasm'] then
            require('jit.dump').on('m', 'jit.log')
        end
    else
        print('! JIT is disabled')
    end

    -- Print some debugdraw info
    print(string.format('%s %s %s', jit.version, jit.os, jit.arch))
    print('JIT active ->')
    print(jit.status())
    print('GC64: ' .. (ffi.abi('gc64') and 'yes' or 'no'))

    collectgarbage_full_cycle()
end

local clock = os.clock
local gc_clock = clock()
local cfg = engine_cfg['system']

-- DO NOT Rename - Invoked from native code
function __on_tick__()
    engine_ctx:tick()
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
