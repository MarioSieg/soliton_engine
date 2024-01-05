-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Core engine setup which runs after boot but before the hook scripts are loaded.

-- Init stack trace plus and redirect debug.traceback to it
local stp = require 'ext.stack_trace_plus'
debug.traceback = stp.stacktrace

-- Init extensions
require 'system.stdextend'

SYSTEM_CFG = require 'system.config' -- Load config
if SYSTEM_CFG.jit_asm_dump then -- Enable JIT ASM dump if requested
    require('jit.dump').on('m')
end

-- Init protocol logger.
protocol = {}

local printProxy = _G.print
function _G.print(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    printProxy(str)
    table.insert(protocol, str)
end

local errorProxy = _G.error
function _G.error(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    errorProxy(str)
    table.insert(protocol, str)
end

function printsep()
    print('------------------------------------------------------------')
end

-- Print package paths
print('Working dir: '..lfs.currentdir())
for _, path in ipairs(INCLUDE_DIRS) do
    print('Additional script dir: ' .. path)
end

-- Verify filesystem
-- TODO must be done before any imports!
print('Verifying filesystem...')
local numchecks = 0
local function checkFsEntry(path)
    numchecks = numchecks + 1
    if not lfs.attributes(path) then
        panic('Broken installation! Required file or directory not found: '..path)
    end
end
checkFsEntry('media')
if lfs.attributes('media/scripts/system/fsregistry.lua') then -- check if the fsregistry file exists
    local REQUIRED_FILES = require 'system.fsregistry' -- load the list of required files
    if type(REQUIRED_FILES) == 'table' then
        for _, path in ipairs(REQUIRED_FILES) do
            checkFsEntry(path)
        end
    end
    print('Filesystem OK, '..numchecks..' entries checked.')
    dofile('media/scripts/tools/fsregistry_gen.lua') -- regenerate the fsregistry file
end

-- Compile shaders
local SHADERC = 'media/scripts/tools/shadercompiler.lua'
if lfs.attributes(SHADERC) then
    dofile(SHADERC)
else
    print('Warning: Shader compiler not found, assuming production build')
    -- TODO check shaders
end

-- Load CDEFS
require 'system.cdefs'

-- Init random seed
math.randomseed(os.time())
for i = 1, math.random() * 10 do -- warm up the random generator
    math.random()
end

local EngineContext = {
    hooks = nil
}

print('Lua mem: '..string.format("%.3f", collectgarbage('count')/1024.0)..' MiB')

function EngineContext:validateHookContext()
    if not self.hooks then
        panic('Failed to load hookmgr, internal error!')
    end
end

function EngineContext:hookModules()
    self.hooks = require 'system.hookmgr'
    self:validateHookContext()
end

function EngineContext:tick()
    self:validateHookContext()
    self.hooks:tick()
end

return EngineContext
