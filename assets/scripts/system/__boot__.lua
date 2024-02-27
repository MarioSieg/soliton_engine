-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file is very important, it's the first script that gets executed by the engine.
-- It's responsible for loading all other scripts and setting up the environment.
-- It also contains the main tick loop.
-- Only modify this file if you know what you're doing.

local jit = require 'jit'
local ffi = require 'ffi'

-- Dont't waste traces, machine-code and other memory jitting any boot code, it's only run once but stays in memory forever.
-- We turn the JIT back on before entering the main tick loop below.
jit.off()

-- We load panic manually because it's not in the package path yet
ffi.cdef [[
    void __lu_panic(const char* msg);
    uint32_t __lu_ffi_cookie(void);
]]

function panic(msg)
    ffi.C.__lu_panic(msg)
end

-- Check that FFI calls work
local cookie = ffi.C.__lu_ffi_cookie()
if cookie ~= 0xfefec0c0 then
    panic('Invalid FFI cookie!')
end

INCLUDE_DIRS = {
    'assets/scripts'
}

-- add all other directories to package paths
for _, path in ipairs(INCLUDE_DIRS) do
    package.path = string.format('%s;%s/?.lua', package.path, path)
end

-- ###################### SYSTEM BOOTSTRAP ######################
-- Everything is now set up to load the engine context

local engineContext = require 'system.setup' -- Lazy load the setup script
assert(engineContext ~= nil)
assert(ENGINE_CONFIG ~= nil)

-- DO NOT Rename - Invoked from native code
-- This function is called before the main tick loop starts.
-- It's responsible for loading all tick hooks of global engine components.
-- Note: Scripts which are loaded per-world are loaded in the world-setup script.
function __on_prepare__()
    engineContext:hookModules() -- Load all start/tick hooks

    if ENGINE_CONFIG.General.enableJit then
        jit.on() -- Enable JIT
        jit.opt.start('+fma') -- enable FMA for better performance
        if ENGINE_CONFIG.General.enableJitAssemblyDump then
            require('jit.dump').on('m', 'jit.log')
        end
    else
        log_info('! JIT is disabled')
    end

    -- Print some debug info
    print(string.format('%s %s %s', jit.version, jit.os, jit.arch))
    print('JIT active ->')
    print(jit.status())

    collectgarbage('collect') -- manually execute GC cycle last time
    collectgarbage('stop') -- stop the GC, we run it manually every frame
end

local clock = os.clock
local gcStartTime = clock()
local gcfg = ENGINE_CONFIG.General

-- DO NOT Rename - Invoked from native code
function __on_tick__()
    if not engineContext then
        panic('Setup script not loaded!')
    end
    engineContext:tick()
    if gcfg.smartFramerateDependentGCStepping then
        local diff = clock() - gcStartTime
        local gcFpsTarget = 1.0/gcfg.targetFramerate
        local timeLeft = gcFpsTarget - diff
        local collectionLim = gcfg.smartFramerateDependentGCSteppingCollectionLimit
        if timeLeft > collectionLim then
            local done = false
            -- -- until garbage is all collected or further collection would exceed the threshold
            while not done and gcFpsTarget - diff > collectionLim do
                done = collectgarbage('step', 1)
                diff = clock() - gcStartTime
            end
        end
    else
        collectgarbage('step') -- manually execute one GC step every frame
    end
    collectgarbage('stop') -- stop the GC because on step resumes it -> we run it manually every frame
end
