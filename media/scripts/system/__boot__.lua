-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file is very important, it's the first script that gets executed by the engine.
-- It's responsible for loading all other scripts and setting up the environment.
-- It also contains the main tick loop.
-- Only modify this file if you know what you're doing.

local ffi = require 'ffi'

-- We load panic manually because it's not in the package path yet
ffi.cdef [[
    void __lu_panic(const char* msg);
    uint32_t __lu_ffi_cookie(void);
]]

function panic(msg)
    assert(type(msg) == 'string')
    ffi.C.__lu_panic(msg)
end

-- Check that FFI calls work
local cookie = ffi.C.__lu_ffi_cookie()
if cookie ~= 0xfefec0c0 then
    panic('Invalid FFI cookie!')
end

INCLUDE_DIRS = {
    'media/scripts'
}

-- add all other directories to package paths
for _, path in ipairs(INCLUDE_DIRS) do
    package.path = string.format('%s;%s/?.lua', package.path, path)
end

local setup = nil

-- DO NOT Rename - Invoked from native code
-- This function is called before the main tick loop starts.
-- It's responsible for loading all tick hooks of global engine components.
-- Note: Scripts which are loaded per-world are loaded in the world-setup script.
function __on_prepare__()
    setup = require 'system.setup' -- Lazy load the setup script
    collectgarbage('stop') -- stop the GC, we run it manually every frame
end

-- DO NOT Rename - Invoked from native code
function __on_tick__()
    if not setup then
        panic('Setup script not loaded!')
    end
    setup:tick()
    collectgarbage('collect') -- manually execute GC cycle every frame
end