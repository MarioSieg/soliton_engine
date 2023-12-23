-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local INCLUDE_DIRS = {
    'media/scripts',
    'media/scripts/lib'
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
local inspect = require 'inspect'
print('JIT active: '..inspect({jit.status()}))

-- Init random seed
math.randomseed(os.time())

-- Uncomment to enable JIT assembly dump (useful for optimizing):

-- require('jit.dump').on('m')

collectgarbage('stop') -- stop the GC, we run it manually every frame

-- Invoked from native code
function __on_update()
    collectgarbage('collect') -- manually execute GC cycle every frame
end
