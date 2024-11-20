----------------------------------------------------------------------------
-- Soliton Engine scene Module
--
-- Copyright (c) 2024 Mario 'Neo' Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local bit = require 'bit'
local app = require 'app'
local entity = require 'entity'
local vec3 = require 'vec3'
local scene_import_flags = require 'detail.import_flags'
local json = require 'json'
local serializer = require 'detail.serializer'
local time = require 'time'

local a_texture = require 'assets.texture'

local band = bit.band

local cpp = ffi.C
ffi.cdef[[
    typedef int lua_scene_id;
    lua_scene_id __lu_scene_create(const char* name);
    lua_scene_id __lu_scene_import(const char* name, const char* model_file, uint32_t load_flags);
    void __lu_scene_tick(void);
    __entity_id __lu_scene_spawn_entity(const char* name);
    void __lu_scene_despawn_entity(__entity_id id);
    __entity_id __lu_scene_get_entity_by_name(const char* name);
    void __lu_scene_full_entity_query_start(void);
    int32_t __lu_scene_full_entity_query_next_table(void);
    __entity_id __lu_scene_full_entity_query_get(int32_t i);
    void __lu_scene_full_entity_query_end(void);
    void __lu_scene_set_active_camera_entity(__entity_id id);
    __entity_id __lu_scene_get_active_camera_entity(void);
    void __lu_scene_set_time(double time);
    void __lu_scene_set_sun_dir(__vec3 sun_dir);
    void __lu_scene_set_sun_color(__vec3 sun_color);
    void __lu_scene_set_ambient_color(__vec3 ambient_color);
    void __lu_scene_set_sky_turbidity(double turbidity);
]]

local scene_format_version = 2

local scene = {
    _id = 0,
    name = 'Default',
    lighting = {
        sun_color = vec3.one,
        ambient_color = vec3.one * 0.1,
        sky_turbidity = 2.2,
    },
    chrono = require 'detail.chrono'
}

--- Loads a texture from the given path.
--- @param path: string, path to the texture file
function scene.load_texture(path)
    return a_texture:load(path)
end

--- Spawns a new entity in the scene with the given name.
--- @param name: string, name of the new entity
function scene.spawn(name)
    return entity:from_native_id(cpp.__lu_scene_spawn_entity(name))
end

--- Despawns the given entity from the scene.
--- @param entity: entity, entity to despawn
function scene.despawn(entity)
    cpp.__lu_scene_despawn_entity(entity._id)
    entity._id = 0
end

--- Looks up an entity in the scene by its name.
--- @param name: string, name of the entity to look up
function scene.lookup_entity(name)
    return entity:from_native_id(cpp.__lu_scene_get_entity_by_name(name))
end

function scene.set_active_camera_entity(camera)
    assert(camera ~= nil)
    cpp.__lu_scene_set_active_camera_entity(camera._id)
end

function scene.get_active_camera_entity()
    return entity:from_native_id(cpp.__lu_scene_get_active_camera_entity())
end

local function setup_scene_class(id, name)
    if type(id) ~= 'number' then
        id = tonumber(id)
    end
    assert(name and type(name) == 'string')

    -- Make scene active
    scene.name = name
    scene._id = id
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

local function serialize_scene_entities()
    local entities = {}
    scene._entity_query_start()
    local num_entities = 0
    for i = 0, scene._entity_query_next() - 1 do
        local ent = scene._entity_query_lookup(i)
        if ent:is_valid() and not ent:has_flag(entity_flags.transient) then
            local name = ent:get_name()
            local acc = 2
            while entities[name] do -- Already set, append a number
                name = name .. '_' .. acc
                acc = acc + 1
            end
            entities[name] = serializer.serialize(ent)
            num_entities = num_entities + 1
        end
    end
    scene._entity_query_end()
    return entities, num_entities
end

function scene.save(file)
    local now = time.hpc_clock_now()
    local entities, num_entities = serialize_scene_entities()
    local data = {
        header = {
            format_version = scene_format_version,
            engine_version = app.engine_version_packed(),
            date = os.date('%Y-%m-%d %H:%M:%S')
        },
        scene = serializer.serialize(scene),
        entities = entities
    }
    json.save(file, data)
    collectgarbage_full_cycle() -- GC to remove all the serialization trash
    print(string.format('Saved scene \'%s\' with %d entities to %s in %.2f ms', scene.name, num_entities, file, time.hpc_clock_elapsed_ms(now)))
end

-- Internal use only.
function scene._update()
    scene.chrono:update()
    -- Update scene time and lighting
    cpp.__lu_scene_set_time(scene.chrono.time)
    cpp.__lu_scene_set_sun_dir(scene.chrono._sun_dir)
    cpp.__lu_scene_set_sun_color(scene.lighting.sun_color * scene.chrono._daytime_coeff)
    cpp.__lu_scene_set_ambient_color(scene.lighting.ambient_color)
    cpp.__lu_scene_set_sky_turbidity(scene.lighting.sky_turbidity)
    cpp.__lu_scene_tick()
end

-- Internal use only.
function scene._entity_query_start()
    cpp.__lu_scene_full_entity_query_start()
end

-- Internal use only.
function scene._entity_query_next()
    return cpp.__lu_scene_full_entity_query_next_table()
end

-- Internal use only.
function scene._entity_query_lookup(i)
    return entity:from_native_id(cpp.__lu_scene_full_entity_query_get(i))
end

-- Internal use only.
function scene._entity_query_end()
    cpp.__lu_scene_full_entity_query_end()
end

return scene
