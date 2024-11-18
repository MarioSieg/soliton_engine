----------------------------------------------------------------------------
-- Soliton Engine scene Module
--
-- Copyright (c) 2024 Mario 'Neo' Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C
local bit = require 'bit'
local entity = require 'entity'
local vec3 = require 'vec3'
local scene_import_flags = require 'import_flags'
local band = bit.band

ffi.cdef[[
    typedef int lua_scene_id;
    lua_scene_id __lu_scene_create(const char* name);
    lua_scene_id __lu_scene_import(const char* name, const char* model_file, uint32_t load_flags);
    void __lu_scene_tick(void);
    lua_entity_id __lu_scene_spawn_entity(const char* name);
    void __lu_scene_despawn_entity(lua_entity_id id);
    lua_entity_id __lu_scene_get_entity_by_name(const char* name);
    void __lu_scene_full_entity_query_start(void);
    int32_t __lu_scene_full_entity_query_next_table(void);
    lua_entity_id __lu_scene_full_entity_query_get(int32_t i);
    void __lu_scene_full_entity_query_end(void);
    void __lu_scene_set_active_camera_entity(lua_entity_id id);
    lua_entity_id __lu_scene_get_active_camera_entity(void);
    void __lu_scene_set_time(double time);
    void __lu_scene_set_sun_dir(lua_vec3 sun_dir);
    void __lu_scene_set_sun_color(lua_vec3 sun_color);
    void __lu_scene_set_ambient_color(lua_vec3 ambient_color);
]]

local lighting = {
    sun_color = vec3.one,
    ambient_color = vec3.one * 0.1
}

local scene = {
    name = 'Default',
    id = 0,
    lighting = lighting,
    clock = require 'game_clock'
}

function scene._update()
    scene.clock:update()
    cpp.__lu_scene_set_time(scene.clock.date.time)
    cpp.__lu_scene_set_sun_dir(scene.clock._sun_dir)
    cpp.__lu_scene_set_sun_color(scene.lighting.sun_color * scene.clock.daytime_coeff)
    cpp.__lu_scene_set_ambient_color(scene.lighting.ambient_color)
    cpp.__lu_scene_tick()
end

function scene.spawn(name)
    return entity:from_native_id(cpp.__lu_scene_spawn_entity(name))
end

function scene.despawn(entity)
    cpp.__lu_scene_despawn_entity(entity.id)
    entity.id = 0
end

function scene.lookup_entity(name)
    return entity:from_native_id(cpp.__lu_scene_get_entity_by_name(name))
end

function scene._entity_query_start()
    cpp.__lu_scene_full_entity_query_start()
end

function scene._entity_query_next()
    return cpp.__lu_scene_full_entity_query_next_table()
end

function scene._entity_query_lookup(i)
    return entity:from_native_id(cpp.__lu_scene_full_entity_query_get(i))
end

function scene._entity_query_end()
    cpp.__lu_scene_full_entity_query_end()
end

function scene.set_active_camera_entity(camera)
    assert(camera ~= nil)
    cpp.__lu_scene_set_active_camera_entity(camera._id)
end

function scene.get_active_camera_entity()
    return entity:from_native_id(cpp.__lu_scene_get_active_camera_entity())
end

local function setup_scene_class(id, name)
    assert(type(id) == 'number' and id > 0)
    assert(name and type(name) == 'string')

    -- Make scene active
    scene.name = name
    scene.id = id
    print(string.format('Created new scene: %s, id: %x', name, id))

    -- Perform one full GC cycle to clean up any garbage
    collectgarbage_full_cycle()
end

--- Creates a new, empty scene with the given name and makes it the active scene.
--- @param name: string, name of the new scene
function scene.new(name)
    name = name or 'Untitled scene'
    local id = cpp.__lu_scene_create(name)
    setup_scene_class(id, name)
end

--- Loads a scene from a given file and makes it the active scene.
--- The file can be a 3d mesh file like .gltf, .fbx, .obj etc.. or a .soliton_engine file.
--- Importing an external file which is not .soliton_engine will take much longer, because conversion is required.
--- @param file: string, path to the file to load
--- @param import_scale: number, scale factor to apply to the imported scene. Only applies if the scene is not a .soliton_engine file
--- @param import_flags: number, flags to control the import process. Only applies if the scene is not a .soliton_engine file
function scene.load(file, import_flags)
    if not file or type(file) ~= 'string' then
        eprint('scene name or source file must be provided')
        return false
    end
    if not import_flags or type(import_flags) ~= 'number' then
        import_flags = scene_import_flags.default
    end
    if not lfs.attributes(file) then
        eprint(string.format('scene file does not exist: %s', file))
        return false
    end
    local name = file:match('^.+/(.+)$'):match('(.+)%..+') -- extract scene name from file path
    local id = cpp.__lu_scene_import(name, file, band(import_flags, 0xffffffff)) -- create native scene
    setup_scene_class(id, name)
end

return scene
