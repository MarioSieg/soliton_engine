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

local function panic(msg)
    assert(type(msg) == 'string')
    ffi.C.__lu_panic(msg)
end

-- Check that FFI calls work
local cookie = ffi.C.__lu_ffi_cookie()
if cookie ~= 0xfefec0c0 then
    panic('Invalid FFI cookie!')
end

local REQUIRED_DIRS = { -- TODO: Maybe autogenerate filelist from real directory?
    'media',
    'media/scripts'
}

-- Let's check if our working directory is correct
local wkdir = lfs.currentdir()
print('Working dir: '..wkdir)
for _, dir in ipairs(REQUIRED_DIRS) do
    local directory = wkdir..'/'..dir
    if lfs.attributes(directory) == nil then
        panic('Broken installation! Required directory not found: '..directory)
    end
end

local INCLUDE_DIRS = {
    'media/scripts'
}

-- add all other directories to package paths
for _, path in ipairs(INCLUDE_DIRS) do
    print('Additional Lua library dirs: ' .. path)
    package.path = string.format('%s;%s/?.lua', package.path, path)
end

-- print JIT info
local jit = require 'jit'
print(jit.version)
print(jit.os)
print(jit.arch)
local inspect = require 'lu.inspect'
print('JIT active: '..inspect({jit.status()}))

-- Init random seed
math.randomseed(os.time())

-- Uncomment to enable JIT assembly dump (useful for optimizing):
-- require('jit.dump').on('m')

-- Ok so now we can load the rest of the engine script files

-- Load all exported scripts
local hooks = {} -- Global variable that holds all tick hooks
local HOOK_DIR = 'media/scripts/lu'

-- DO NOT Rename - Invoked from native code
function __on_prepare__()
    local files = {}
    for file in lfs.dir(HOOK_DIR) do
        if file ~= '.' and file ~= '..' then
            local path = HOOK_DIR..'/'..file
            local attr = lfs.attributes(path)
            if attr.mode == 'file' then
                table.insert(files, path)
            end
        end
    end
    table.sort(files)
    for _, file in ipairs(files) do
        print('Loading script: '..file)
        local chunk, err = loadfile(file)
        if chunk == nil then
            panic('Failed to load script: '..file..'\n'..err)
        end
        print('Executing script: '..file)
        local mod = chunk() -- execute the script
        local hook = mod._tick_
        if hook ~= nil then
            table.insert(hooks, hook)
        end
    end
    print('Loaded '..#hooks..' tick hooks')
    collectgarbage('stop') -- stop the GC, we run it manually every frame
end

-- DO NOT Rename - Invoked from native code
function __on_tick__()
    for _, hook in ipairs(hooks) do
        hook() -- execute all tick hooks
    end
    collectgarbage('collect') -- manually execute GC cycle every frame
end
