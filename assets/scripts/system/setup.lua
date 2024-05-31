-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Core engine setup which runs after boot but before the hook scripts are loaded.

-- Init stack trace plus and redirect debugdraw.traceback to it
local stp = require 'ext.stack_trace_plus'
debug.traceback = stp.stacktrace

-- Init protocol logger.
protocol = {}
protocol_errs = 0

local print_proxy = _G.print
function _G.print(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, false})
    print_proxy(str)
end

function eprint(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, true})
    protocol_errs = protocol_errs + 1
end

local error_proxy = _G.error
function _G.error(...)
    local args = {...}
    local str = string.format('[%s] ', os.date('%H:%M:%S'))
    for i, arg in ipairs(args) do
        if i > 1 then
            str = str..' '
        end
        str = str..tostring(arg)
    end
    table.insert(protocol, {str, true})
    protocol_errs = protocol_errs + 1
    error_proxy(str)
end

function printsep()
    print('------------------------------------------------------------')
end

-- Init misc

-- Load global engine config
require 'config.engine'

if engine_cfg.General.loadLuaStdlibExtensions then
    -- Init extensions
    require 'system.stdextend'
end

-- Print package paths
print('Working dir: '..lfs.currentdir())
for _, path in ipairs(lua_include_dirs) do
    print('Additional script dir: ' .. path)
end

-- Verify filesystem
if engine_cfg.General.enableFilesystemValidation then
    print('Verifying filesystem...')
    local numchecks = 0
    local function checkFsEntry(path)
        numchecks = numchecks + 1
        if not lfs.attributes(path) then
            panic('Broken installation! Required file or directory not found: '..path)
        end
    end
    checkFsEntry('assets')
    if lfs.attributes('assets/scripts/system/fsregistry.lua') then -- check if the fsregistry file exists
        local REQUIRED_FILES = require 'system.fsregistry' -- load the list of required files
        if type(REQUIRED_FILES) == 'table' then
            for _, path in ipairs(REQUIRED_FILES) do
                checkFsEntry(path)
            end
        end
        print('Filesystem OK, '..numchecks..' entries checked.')
        dofile('assets/scripts/tools/fsregistry_gen.lua') -- regenerate the fsregistry file
    end
end

-- Load CDEFS
require 'system.cdefs'

-- Init random seed
math.randomseed(os.time())
for i = 1, math.random() * 10 do -- warm up the random generator
    math.random()
end

local engine_ctx = {
    hooks = nil
}

print(string.format('Lua mem limit: %.3f MiB', collectgarbage('count')/1024.0))

function engine_ctx:validate()
    if not self.hooks then
        panic('Failed to load hookmgr, internal error!')
    end
end

function engine_ctx:inject_hooks()
    self.hooks = require 'system.hookmgr'
    self:validate()
end

function engine_ctx:tick()
    self:validate()
    self.hooks:tick()
end

return engine_ctx
