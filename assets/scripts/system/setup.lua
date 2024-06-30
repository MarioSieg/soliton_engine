-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- Core engine setup which runs after boot but before the hook scripts are loaded.

-- Load global engine config
require 'system.extensions'
require 'config.engine'

-- Print package paths
print('Working dir: '..lfs.currentdir())
for _, path in ipairs(lua_include_dirs) do
    print('Additional script dir: ' .. path)
end

-- Verify filesystem
if engine_cfg.General.enableFilesystemValidation then
    print('Verifying filesystem...')
    local numchecks = 0
    local function check_fs_entry(path)
        numchecks = numchecks + 1
        if not lfs.attributes(path) then
            panic('Broken installation! Required file or directory not found: '..path)
        end
    end
    check_fs_entry('assets')
    if lfs.attributes('assets/scripts/system/fsregistry.lua') then -- check if the fsregistry file exists
        local REQUIRED_FILES = require 'system.fsregistry' -- load the list of required files
        if type(REQUIRED_FILES) == 'table' then
            for _, path in ipairs(REQUIRED_FILES) do
                check_fs_entry(path)
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
