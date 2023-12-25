-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Core engine setup which runs after boot but before the hook scripts are loaded.

-- Set to true to enable JIT assembly dump (useful for optimizing):
JIT_ASM_DUMP = false
if JIT_ASM_DUMP then
    require('jit.dump').on('m')
end

-- Init protocol logger
protocol = {}
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
    print_proxy(str)
    table.insert(protocol, str)
end

-- print JIT info
local jit = require 'jit'
local inspect = require 'lu.inspect'
print(string.format('%s %s %s', jit.version, jit.os, jit.arch))
local status = tostring(jit.status())
print('JIT active: '..status)

-- print package paths
print('Working dir: '..lfs.currentdir())
for _, path in ipairs(INCLUDE_DIRS) do
    print('Additional script dir: ' .. path)
end

-- verify filesystem
print('Verifying filesystem...')
local numchecks = 0
local function fs_check(path)
    numchecks = numchecks + 1
    if not lfs.attributes(path) then
        panic('Broken installation! Required file or directory not found: '..path)
    end
end
fs_check('media')
fs_check('media/scripts/system/fsregistry.lua') -- this file is required to check the other files
local REQUIRED_FILES = require 'system.fsregistry' -- load the list of required files
for _, path in ipairs(REQUIRED_FILES) do
    fs_check(path)
end
print('Filesystem OK, '..numchecks..' entries checked.')
dofile 'media/scripts/system/fsregistry_gen.lua' -- regenerate the fsregistry file

-- Init random seed
math.randomseed(os.time())
for i = 1, math.random() * 10 do -- warm up the random generator
    math.random()
end

local M = {
    hooks = require 'system.hookmgr'
}

function M:tick()
    self.hooks:tick()
end

return M
