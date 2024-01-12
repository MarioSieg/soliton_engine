-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local FORCE_SHADER_RECOMPILATION = false -- set to true to force shader recompilation
local VERBOSE = false -- set to true to enable verbose shader compilation
local PARALLEL = true -- set to true to enable parallel shader compilation

local now = os.clock()
local xml = require 'ext.xml'

local EXECUTABLE = 'tools/shaderc_'
local OUT_DIR = 'media/compiledshaders/'
local REGISTRY = OUT_DIR..'registry.xml'

print('Shader compiler script started.')
print('Checking for modified shader files...')

-- Detect if there are new shaders to compile and compile them if necessary.

-- Recursive function to get the modification time of each file in a directory and its subdirectories
local function getFileModificationTimes(directory, times)
    times = times or {}
    for file in lfs.dir(directory) do
        if file ~= "." and file ~= ".." then
            local path = directory .. '/' .. file
            local attr = lfs.attributes(path)
            if attr.mode == "file" then
                times[path] = tostring(attr.modification)
            elseif attr.mode == "directory" then
                getFileModificationTimes(path, times) -- Recursive call for subdirectories
            end
        end
    end
    return times
end

-- Check if any file has changed
local function checkForChanges(oldTimes, newTimes)
    for file, time in pairs(newTimes) do
        if not oldTimes[file] or oldTimes[file] ~= time then
            return true, file
        end
    end
    return false
end

-- So if the registry exists, we check if any file has changed
if lfs.attributes(REGISTRY) then
    local handler = require 'ext.xml_tree'
    local parser = xml.parser(handler)
    parser:parse(xml.loadFile(REGISTRY))
    if not handler.root or not handler.root.shader_registry then
        panic('Invalid shader registry file: '..REGISTRY)
        FORCE_SHADER_RECOMPILATION = true
    else
        local oldTimes = handler.root.shader_registry
        local newTimes = getFileModificationTimes('shaders')
        local changed, file = checkForChanges(oldTimes, newTimes)
        if changed then
            print('Shader file '..file..' has changed. Recompiling shaders...')
            FORCE_SHADER_RECOMPILATION = true
        end
    end
end

if lfs.attributes(OUT_DIR) and not FORCE_SHADER_RECOMPILATION then
    print('Shader compilation skipped, already compiled and no force recompilation flag set.')
    return
end

-- delete output directory:
print('Deleting output directory '..OUT_DIR)
for file in lfs.dir(OUT_DIR) do
    if file ~= '.' and file ~= '..' then
        local path = OUT_DIR..'/'..file
        if lfs.attributes(path).mode == 'file' then
            os.remove(path)
        end
    end
end
lfs.rmdir(OUT_DIR)

-- create output directory:
lfs.mkdir(OUT_DIR)
OUT_DIR = OUT_DIR..(jit.os:lower())..'/' -- append platform name
lfs.mkdir(OUT_DIR)
print('Output directory: '..OUT_DIR)

-- Now we update the registry
print('Updating shader registry...')
local newTimes = getFileModificationTimes('shaders')
if not lfs.attributes(OUT_DIR) then
    lfs.mkdir(OUT_DIR)
end
if not lfs.attributes(REGISTRY) then
    lfs.touch(REGISTRY)
end
local file = io.open(REGISTRY, 'w')
file:write(xml.toXml(newTimes, 'shader_registry'))
file:close()

printsep()

if jit.arch ~= 'x64' then
    panic('Unsupported architecture. Supported architectures are AMD64.')
end

EXECUTABLE = EXECUTABLE..(jit.os:lower())..'_amd64'
if jit.os == 'Windows' then
    EXECUTABLE = EXECUTABLE..'.exe'
end

if not lfs.attributes(EXECUTABLE) then
    panic('Shader compiler executable not found: '..EXECUTABLE)
end

print('Shader compiler executable: '..EXECUTABLE)

if jit.os == 'Windows' then -- Windows needs to execute the compiler in the current directory
    EXECUTABLE = string.format('cmd.exe /c \"%s/%s\"', lfs.currentdir(), EXECUTABLE)
end

local SHADER_TYPE = {
    VERTEX = 0,
    FRAGMENT = 1,
    COMPUTE = 2
}

local SHADER_TYPE_NAMES = {
    [SHADER_TYPE.VERTEX] = 'vertex',
    [SHADER_TYPE.FRAGMENT] = 'fragment',
    [SHADER_TYPE.COMPUTE] = 'compute'
}

local SHADER_TYPE_PREFIX = {
    ['vs'] = SHADER_TYPE.VERTEX,
    ['fs'] = SHADER_TYPE.FRAGMENT,
    ['cs'] = SHADER_TYPE.COMPUTE
}

local SHADER_TARGET = {
    DX11 = 0,
    DX12 = 1,
    VULKAN = 2,
    METAL = 3,
    GLSL = 4
}

local SHADER_TARGET_NAMES = {
    [SHADER_TARGET.DX11] = 'dx11',
    [SHADER_TARGET.DX12] = 'dx12',
    [SHADER_TARGET.VULKAN] = 'vulkan',
    [SHADER_TARGET.METAL] = 'metal',
    [SHADER_TARGET.GLSL] = 'gl'
}

local SHADER_TARGET_PROFILES = {
    [SHADER_TARGET.DX11] = 's_5_0',
    [SHADER_TARGET.DX12] = 's_5_0',
    [SHADER_TARGET.VULKAN] = 'spirv',
    [SHADER_TARGET.METAL] = 'metal',
    [SHADER_TARGET.GLSL] = '150'
}

local PLATFORM_TARGETS = {
    ['Windows'] = {SHADER_TARGET.DX11, SHADER_TARGET.DX12, SHADER_TARGET.VULKAN},
    ['Linux'] = {SHADER_TARGET.VULKAN, SHADER_TARGET.GLSL},
    ['OSX'] = {SHADER_TARGET.METAL}
}

local VARYING_DEF = 'def.sc'
local STDLIB_INCLUDE = 'shaders/lib'

-- Following shaders must be compiled:
-- Windows: DX11, Vulkan - DX12 uses the same shaders
-- Linux: Vulkan, OpenGL
-- macOS: Metal

-- first we search for all subdirs in the shaders/folder:
local shaderDirs = {}
print('Searching for shader modules...')
for dir in lfs.dir('shaders') do
    if dir ~= '.' and dir ~= '..' then
        local path = 'shaders/'..dir
        if lfs.attributes(path).mode == 'directory' then
            table.insert(shaderDirs, path)
        end
    end
end

local function getFileName(file)
    local name = file:match("[^/]*.sc$")
    return name:sub(0, #name-3)
end

-- now we search for all shader files in the subdirs:
local shaderFiles = {}
for _, dir in ipairs(shaderDirs) do
    local files = {
        dir = dir,
        sources = {}
    }
    for file in lfs.dir(dir) do
        if file ~= '.' and file ~= '..' then
            local path = dir..'/'..file
            if string.sub(path, -3) == '.sc' and lfs.attributes(path).mode == 'file' then
                table.insert(files.sources, path)
            else
                print('Warning: Ignoring file '..path)
            end
        end
    end
    if #files.sources > 0 then
        table.insert(shaderFiles, files)
    end
end

-- now we compile all shaders:
print('Compiling '..#shaderFiles..' shader module(s)...')

local WKDIR = lfs.currentdir()..'/'

local function compileShader(base, file, varying, stype, target)
    local apiDir = string.format('%s%s/', OUT_DIR, SHADER_TARGET_NAMES[target])
    lfs.mkdir(apiDir)
    apiDir = apiDir..base
    lfs.mkdir(apiDir)
    local type = SHADER_TYPE_NAMES[stype]
    local profile = SHADER_TARGET_PROFILES[target]
    file = WKDIR..file
    local out = WKDIR..string.format('%s/%s.bin', apiDir, getFileName(file))
    local exec = EXECUTABLE
    local cmd = string.format('%s -f %s -o %s --platform %s --type %s --profile %s --varyingdef %s -i %s', exec, file, out, jit.os:lower(), type, profile,  WKDIR..varying,  WKDIR..STDLIB_INCLUDE)
    if PARALLEL then
        if jit.os == 'Windows' then
            cmd = 'start /b '..cmd -- start in new window
        else
            cmd = cmd..' &' -- run in background
        end
    end
    if VERBOSE then
        cmd = cmd..' --verbose'
    end
    if target == SHADER_TARGET.DX11 or target == SHADER_TARGET.DX12 then -- DX11 and DX12 should be compiled with optimization level 3
        cmd = cmd..' -O3'
    end
    print(cmd)
    local ok = os.execute(cmd)
    if ok ~= 0 then
        panic('Shader compilation failed. Code: '..tostring(code)..' Command: '..cmd)
    end
end

local function getParentPath(path)
    return path:match("([^/]+)/?$")
end

for _, files in ipairs(shaderFiles) do
    local varying = files.dir..'/'..VARYING_DEF
    if not lfs.attributes(varying) then
        panic('Varying definition file not found: '..varying)
    end
    local base = getParentPath(files.dir)
    for _, file in ipairs(files.sources) do
        local fName = getFileName(file)
        if not fName:find('^def') then -- ignore varying definition file
            print('Compiling '..file..'... for '..(#PLATFORM_TARGETS[jit.os])..' target(s)')
            local stype = nil
            for prefix, type in pairs(SHADER_TYPE_PREFIX) do
                if  fName:find('^'..prefix) then
                    stype = type
                    break
                end
            end
            if not stype then
                panic('Unknown shader type: '..file..' (must start with vs, fs or cs)')
            end
            for _, target in ipairs(PLATFORM_TARGETS[jit.os]) do
                compileShader(base, file, varying, stype, target)
            end
        end
    end
end

print('Shader compilation finished. Time: '..(os.clock()-now)..'s')

printsep()
