-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- TODO: Fix

local xml = require 'ext.xml'

local project = { -- project structure, NO tables allowed in this class because of serialization
    serialized = { -- project meta data - everything within here is serialized to the lupro file
        name = 'Unnamed project',
        id = 0,
        version = '0.0.1',
        type = 'game',
        author = 'Unknown',
        description = 'Default',
        license = '',
        copyright = '',
        url = 'www.example.com',
        asset_dir = 'assets',
        config_dir = 'config',
        hook_dir = 'hooks',
        tmp_dir = 'tmp',
        created_time_stamp = nil,
        modifier_time_stamp = nil,
        load_accumulator = 0
    },
    full_path = nil
}

function project:new(dir, name)
    if dir then assert(type(dir) == 'string') end
    if name then assert(type(name) == 'string') end
    local project = {}
    setmetatable(project, {__index = self})   
    project.serialized.name = name or 'Unnamed project'
    project.full_path = dir
    project.serialized.id = os.time()
    local stamp = os.date('%Y-%m-%d %H:%M:%S')
    project.serialized.created_time_stamp = stamp
    project.serialized.modifier_time_stamp = stamp
    return project
end

function project:getProjectFile()
    if not self.full_path then
        return nil
    end
    return self.full_path..'/project.lupro'
end

function project:saveMetaDataToFile()
    local target = self:getProjectFile()
    if not target then
        error('No project file specified')
    end
    if lfs.attributes(target) then
        lfs.remove(target)
    end
    local file = io.open(target, 'w')
    file:write(xml.toXml(self.serialized, 'LunamProject'))
    file:close()
end

function project:loadMetaDataFromFile()
    local target = self:getProjectFile()
    if not target then
        error('No project file specified')
    end
    if not lfs.attributes(target) then
        error('project file not found: '..target)
    end
    local file = io.open(target, 'r')
    local content = file:read('*a')
    file:close()
    local handler = require 'ext.xml_tree'
    local parser = xml.parser(handler)
    parser:parse(content)
    local target = handler.root.LunamProject
    if not target or type(target) ~= 'table' then
        error('Failed to parse XML project file: '..target)
    end
    self.serialized = target
end

function project:open(projectRootFile)
    if not projectRootFile then
        error('No project file specified')
    end
    if not lfs.attributes(projectRootFile) then
        error('project file not found: '..projectRootFile)
    end
    if not projectRootFile:endsWith('.lupro') then
        error('Invalid project file extension: '..projectRootFile)
    end
    -- Extract project root directory from file path
    local rootDir = projectRootFile:match('^(.*)/')
    if not rootDir then
        error('Failed to extract project root directory from: '..projectRootFile)
    end
    if not lfs.attributes(rootDir) then
        error('project root directory not found: '..rootDir)
    end
    local project = project:new(rootDir, nil)
    project:loadMetaDataFromFile()
    project.serialized.modifier_time_stamp = os.date('%Y-%m-%d %H:%M:%S')
    project.serialized.load_accumulator = project.serialized.load_accumulator + 1
    return project
end

function project:getAssetDir()
    return self.asset_dir
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

function project:createOnDisk(rootDir)
    if not lfs.attributes(rootDir) then
        lfs.mkdir(rootDir)
    end
    local fullPath = rootDir
    if rootDir:match("([^/]+)$") ~= self.serialized.name then
        fullPath = fullPath..'/'..self.serialized.name
    end
    if lfs.attributes(fullPath) then
        error('project already exists: '..fullPath)
    end
    lfs.mkdir(fullPath)
    if not lfs.attributes(fullPath) then
        error('Failed to create project directory: '..fullPath)
    end
    if not lfs.attributes(PROJECT_TEMPLATE_DIR) then
        error('project template directory not found: '..PROJECT_TEMPLATE_DIR)
    end
    copyDir(PROJECT_TEMPLATE_DIR, fullPath)
    if not lfs.attributes(fullPath) then
        error('Failed to copy project template to: '..fullPath)
    end
    self.full_path = fullPath
    self:saveMetaDataToFile()
end

return project
