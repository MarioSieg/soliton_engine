-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local json = require 'json'

local template_dir = 'templates/project'

-- TODO: move this to a separate file
local function copy_file(src, dst)
    local file_in, err = io.open(src, 'rb')
    if not file_in then
        error('Failed to open source file: ' .. err)
    end
    local file_out, err = io.open(dst, 'wb')
    if not file_out then
        error('Failed to open destination file: ' .. err)
    end
    local content = file_in:read('*a') -- Read the entire content of the source file
    file_out:write(content) -- Write the content to the destination file
    file_in:close()
    file_out:close()
end

-- TODO: move this to a separate file
local function copy_dir_recursive(srcDir, dstDir)
    lfs.mkdir(dstDir) -- Make sure the destination directory exists
    for entry in lfs.dir(srcDir) do
        if entry ~= '.' and entry ~= '..' then
            local src = srcDir .. '/' .. entry
            local dst = dstDir .. '/' .. entry
            local mode = lfs.attributes(src, 'mode')
            if mode == 'directory' then
                copy_dir_recursive(src, dst) -- Recursively copy subdirectories
            elseif mode == 'file' then
                copy_file(src, dst) -- Copy files
            end
        end
    end
end

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

--- Create new project on disk with template files.
function project:create(dir, name)
    if dir then assert(type(dir) == 'string') end
    if name then assert(type(name) == 'string') end
    local proj = {}
    setmetatable(proj, {__index = self})
    proj.serialized.name = name or 'Unnamed project'
    proj.full_path = dir
    proj.serialized.id = os.time()
    local stamp = os.date('%Y-%m-%d %H:%M:%S')
    proj.serialized.created_time_stamp = stamp
    proj.serialized.modifier_time_stamp = stamp
    if not lfs.attributes(dir) then
        lfs.mkdir(dir)
    end
    local full_path = dir
    if dir:match("([^/]+)$") ~= proj.serialized.name then
        full_path = full_path .. '/' .. proj.serialized.name
    end
    if lfs.attributes(full_path) then
        error('project already exists: ' .. full_path)
    end
    lfs.mkdir(full_path)
    if not lfs.attributes(full_path) then
        error('Failed to create project directory: ' .. full_path)
    end
    if not lfs.attributes(template_dir) then
        error('project template directory not found: ' .. template_dir)
    end
    copy_dir_recursive(template_dir, full_path)
    if not lfs.attributes(full_path) then
        error('Failed to copy project template to: ' .. full_path)
    end
    proj.full_path = full_path
    proj:_serialize_config_to_lupro()
    return proj
end

--- Open existing project on disk.
function project:open(lupro_file)
    if not lupro_file then
        error('No project file specified')
    end
    if not lfs.attributes(lupro_file) then
        error('project file not found: ' .. lupro_file)
    end
    if not lupro_file:ends_with('.lupro') then
        error('Invalid project file extension: ' .. lupro_file)
    end
    -- Extract project root directory from file path
    local parent_dir = lupro_file:match('^(.*)/')
    if not parent_dir then
        error('Failed to extract project root directory from: ' .. lupro_file)
    end
    if not lfs.attributes(parent_dir) then
        error('project root directory not found: ' .. parent_dir)
    end
    local proj = {}
    setmetatable(proj, { __index = self })
    proj.full_path = parent_dir
    proj:_deserialize_config_from_lupro()
    proj.serialized.modifier_time_stamp = os.date('%Y-%m-%d %H:%M:%S')
    proj.serialized.load_accumulator = proj.serialized.load_accumulator + 1
    proj:_execute_load_hook()
    return proj
end

function project:unload()
    self:_execute_unload_hook()
    self:_serialize_config_to_lupro()
end

function project:get_lupro_file_path()
    assert(self.full_path)
    return self.full_path..'/project.lupro'
end

function project:_execute_load_hook()
    assert(self.full_path)
    local file = string.format('%s/%s/load.lua', self.full_path, self.serialized.hook_dir)
    print('Executing load hook: '..file)
    if not lfs.attributes(file) then
        eprint('Failed to execute load hook, file not found: '..file)
        return
    end
    return dofile(file)
end

function project:_execute_unload_hook()
    assert(self.full_path)
    local file = string.format('%s/%s/unload.lua', self.full_path, self.serialized.hook_dir)
    print('Executing unload hook: '..file)
    if not lfs.attributes(file) then
        eprint('Failed to execute load hook, file not found: '..file)
        return
    end
    return dofile(file)
end

function project:_serialize_config_to_lupro()
    local target = self:get_lupro_file_path()
    if lfs.attributes(target) then
        lfs.remove(target)
    end
    json.serialize_to_file(target, self.serialized)
end

function project:_deserialize_config_from_lupro()
    local target = self:get_lupro_file_path()
    if not lfs.attributes(target) then
        error('project file not found: '..target)
    end
    local deserialized_data = json.deserialize_from_file(target)
    if not deserialized_data or type(deserialized_data) ~= 'table' then
        error('Failed to parse XML project file: '..target)
    end
    self.serialized = deserialized_data
end

return project
