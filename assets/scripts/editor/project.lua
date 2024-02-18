-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local xml = require 'ext.xml'

local Project = { -- Project structure, NO tables allowed in this class because of serialization
    serialized = {
        name = 'Unnamed Project',
        id = 0,
        version = '0.0.1',
        type = 'game',
        author = 'Unknown',
        description = 'Default',
        license = 'MIT',
        copyright = '',
        url = 'www.example.com',
        assetDir = 'assets',
        configDir = 'config',
        hookDir = 'hooks',
        tmpDir = 'tmp',
        createdTimeStamp = nil,
        lastModifiedTimeStamp = nil,
        loadAccumulator = 0
    },
    transientRootDir = nil,
    transientFullPath = nil
}

function Project:new(dir, name)
    if dir then assert(type(dir) == 'string') end
    if name then assert(type(name) == 'string') end
    local o = {}
    setmetatable(o, {__index = self})   
    o.serialized.name = name or 'Unnamed Project'
    o.transientRootDir = dir
    o.serialized.id = os.time()
    local timeStampString = os.date('%Y-%m-%d %H:%M:%S')
    o.serialized.createdTimeStamp = timeStampString
    o.serialized.lastModifiedTimeStamp = timeStampString
    return o
end

function Project:getAssetDir()
    return self.assetDir
end

local PROJECT_TEMPLATE_DIR = 'templates/project'

local function dirtree(dir)
    local dirs = {}
    local function yieldtree(dir)
        for entry in lfs.dir(dir) do
            if entry ~= '.' and entry ~= '..' then
                local path = dir..'/'..entry
                local attribs = lfs.attributes(path)
                if attribs.mode == 'directory' then
                    table.insert(dirs, path)
                    yieldtree(path)
                else
                    coroutine.yield(path)
                end
            end
        end
    end
    return coroutine.wrap(function() yieldtree(dir) end), dirs
end

local function copyFile(src, dst)
    local inFile, err = io.open(src, 'rb')
    if not inFile then
        error('Failed to open source file: ' .. err)
    end
    local outFile, err = io.open(dst, 'wb')
    if not outFile then
        error('Failed to open destination file: ' .. err)
    end
    local content = inFile:read('*a') -- Read the entire content of the source file
    outFile:write(content) -- Write the content to the destination file 
    inFile:close()
    outFile:close()
end

local function copyDir(srcDir, dstDir)
    lfs.mkdir(dstDir) -- Make sure the destination directory exists
    for entry in lfs.dir(srcDir) do
        if entry ~= '.' and entry ~= '..' then
            local srcPath = srcDir .. '/' .. entry
            local dstPath = dstDir .. '/' .. entry
            local mode = lfs.attributes(srcPath, 'mode')
            if mode == 'directory' then
                copyDir(srcPath, dstPath) -- Recursively copy subdirectories
            elseif mode == 'file' then
                copyFile(srcPath, dstPath) -- Copy files
            end
        end
    end
end

function Project:createOnDisk()
    if not lfs.attributes(self.transientRootDir) then
        lfs.mkdir(self.transientRootDir)
    end
    local fullPath = self.transientRootDir..'/'..self.serialized.name
    if lfs.attributes(fullPath) then
        error('Project already exists: '..fullPath)
    end
    lfs.mkdir(fullPath)
    if not lfs.attributes(fullPath) then
        error('Failed to create project directory: '..fullPath)
    end
    if not lfs.attributes(PROJECT_TEMPLATE_DIR) then
        error('Project template directory not found: '..PROJECT_TEMPLATE_DIR)
    end
    copyDir(PROJECT_TEMPLATE_DIR, fullPath)
    if not lfs.attributes(fullPath) then
        error('Failed to copy project template to: '..fullPath)
    end
    self.transientFullPath = fullPath
    if not self:saveMetaDataToFile() then
        error('Failed to save project metadata to: '..fullPath)
    end
end

function Project:getProjectFile()
    if not self.transientFullPath then
        return nil
    end
    return self.transientFullPath..'/project.lupro'
end

function Project:saveMetaDataToFile()
    local target = self:getProjectFile()
    if not target then
        return false
    end
    if lfs.attributes(target) then
        lfs.remove(target)
    end
    local file = io.open(target, 'w')
    file:write(xml.toXml(self.serialized, 'LunamProject'))
    file:close()
    return true
end

return Project
