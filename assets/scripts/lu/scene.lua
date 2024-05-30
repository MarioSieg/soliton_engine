----------------------------------------------------------------------------
-- Lunam Engine scene Module
--
-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local C = ffi.C
local bit = require 'bit'
local bor = bit.bor
local band = bit.band
local entity = require 'lu.entity'
local components = require 'lu.components'

ffi.cdef[[
    typedef int lua_scene_id;
    lua_scene_id __lu_scene_create(const char* name);
    lua_scene_id __lu_scene_import(const char* name, const char* model_file, double scale, uint32_t load_flags);
    void __lu_scene_tick(void);
    void __lu_scene_start(void);
    lua_entity_id __lu_scene_spawn_entity(const char* name);
    void __lu_scene_despawn_entity(lua_entity_id id);
    lua_entity_id __lu_scene_get_entity_by_name(const char* name);
    void __lu_scene_full_entity_query_start(void);
    int32_t __lu_scene_full_entity_query_next_table(void);
    lua_entity_id __lu_scene_full_entity_query_get(int32_t i);
    void __lu_scene_full_entity_query_end(void);
    void __lu_scene_set_active_camera_entity(lua_entity_id id);
    lua_entity_id __lu_scene_get_active_camera_entity(void);
]]

local function flagSetUnion(flags)
    local result = 0
    for i=1, #flags do
        result = bor(result, flags[i])
    end
    return band(result, 0xffffffff)
end

--- scene import flags for post processing.
--- Only applies if the scene is not a .lunam file.
SCENE_IMPORT_FLAGS = {
    NONE = 0x0,
    CALC_TANGENT_SPACE = 0x1,
    JOIN_IDENTICAL_VERTICES = 0x2,
    MAKE_LEFT_HANDED = 0x4,
    TRIANGULATE = 0x8,
    REMOVE_COMPONENT = 0x10,
    GEN_NORMALS = 0x20,
    GEN_SMOOTH_NORMALS = 0x40,
    SPLIT_LARGE_MESHES = 0x80,
    PRE_TRANSFORM_VERTICES = 0x100,
    LIMIT_BONE_WEIGHTS = 0x200,
    VALIDATE_DATA_STRUCTURE = 0x400,
    IMPROVE_CACHE_LOCALITY = 0x800,
    REMOVE_REDUNDANT_MATERIALS = 0x1000,
    FIX_INFACING_NORMALS = 0x2000,
    POPULATE_ARMATURE_DATA = 0x4000,
    SORT_BY_PTYPE = 0x8000,
    FIND_DEGENERATES = 0x10000,
    FIND_INVALID_DATA = 0x20000,
    GEN_UV_COORDS = 0x40000,
    TRANSFORM_UV_COORDS = 0x80000,
    FIND_INSTANCES = 0x100000,
    OPTIMIZE_MESHES = 0x200000,
    OPTIMIZE_GRAPH = 0x400000,
    FLIP_UVS = 0x800000,
    FLIP_WINDING_ORDER = 0x1000000,
    SPLIT_BY_BONE_COUNT = 0x2000000,
    DEBONE = 0x4000000,
    GLOBAL_SCALE = 0x8000000,
    EMBED_TEXTURES = 0x10000000,
    FORCE_GEN_NORMALS = 0x20000000,
    DROP_NORMALS = 0x40000000,
    GEN_BOUNDING_BOXES = 0x80000000,
}

SCENE_IMPORT_FLAGS.PRESET_REALTIME_QUALITY = flagSetUnion({
    SCENE_IMPORT_FLAGS.CALC_TANGENT_SPACE,
    SCENE_IMPORT_FLAGS.GEN_SMOOTH_NORMALS,
    SCENE_IMPORT_FLAGS.JOIN_IDENTICAL_VERTICES,
    SCENE_IMPORT_FLAGS.IMPROVE_CACHE_LOCALITY,
    SCENE_IMPORT_FLAGS.LIMIT_BONE_WEIGHTS,
    SCENE_IMPORT_FLAGS.REMOVE_REDUNDANT_MATERIALS,
    SCENE_IMPORT_FLAGS.TRIANGULATE,
    SCENE_IMPORT_FLAGS.GEN_UV_COORDS,
    SCENE_IMPORT_FLAGS.SORT_BY_PTYPE,
    SCENE_IMPORT_FLAGS.FIND_DEGENERATES,
    SCENE_IMPORT_FLAGS.FIND_INVALID_DATA,
    SCENE_IMPORT_FLAGS.FIND_INSTANCES,
    SCENE_IMPORT_FLAGS.OPTIMIZE_MESHES,
    -- SCENE_IMPORT_FLAGS.OPTIMIZE_GRAPH
})

SCENE_IMPORT_FLAGS.PRESET_CONVERT_TO_LH = flagSetUnion({
    SCENE_IMPORT_FLAGS.MAKE_LEFT_HANDED,
    SCENE_IMPORT_FLAGS.FLIP_UVS,
    SCENE_IMPORT_FLAGS.FLIP_WINDING_ORDER
})

SCENE_IMPORT_FLAGS.DEFAULT_FLAGS = flagSetUnion({
    SCENE_IMPORT_FLAGS.PRESET_REALTIME_QUALITY ,
    SCENE_IMPORT_FLAGS.PRESET_CONVERT_TO_LH
})

local scene = {
    name = 'Untitled',
    id = 0
}

function scene.__onStart()
    C.__lu_scene_start()
end

function scene.__onTick()
    C.__lu_scene_tick()
end

function scene.spawn(name)
    return entity:fromId(C.__lu_scene_spawn_entity(name))
end

function scene.despawn(entity)
    C.__lu_scene_despawn_entity(entity.id)
    entity.id = nil
end

function scene.getEntityByName(name)
    return entity:fromId(C.__lu_scene_get_entity_by_name(name))
end

function scene.fullEntityQueryStart()
    C.__lu_scene_full_entity_query_start()
end

function scene.fullEntityQueryNextTable()
    return C.__lu_scene_full_entity_query_next_table()
end

function scene.fullEntityQueryGet(i)
    return entity:fromId(C.__lu_scene_full_entity_query_get(i))
end

function scene.fullEntityQueryEnd()
    C.__lu_scene_full_entity_query_end()
end

function scene.setActiveCameraEntity(entity)
    C.__lu_scene_set_active_camera_entity(entity.id)
end

function scene.getActiveCameraEntity()
    return entity:fromId(C.__lu_scene_get_active_camera_entity())
end

local function setLocalSceneProps(id, sceneName)
    -- Check if the id is valid
    if type(id) ~= 'number' or id == 0 then
        eprint(string.format('Failed to create scene, invalid id returned: %s', sceneName))
        return false
    end

    assert(sceneName and type(sceneName) == 'string')

    -- Make scene active
    scene.name = sceneName
    scene.id = id
    scene.__onStart() -- invoke start hook
    print(string.format('Created new scene: %s, id: %x', sceneName, id))

    -- Perform one full GC cycle to clean up any garbage
    collectgarbage('collect')
    collectgarbage('stop')
    return true
end

--- Creates a new, empty scene with the given name and makes it the active scene.
--- @param name: string, name of the new scene
function scene.new(sceneName)
    sceneName = sceneName or 'Untitled scene'
    local id = C.__lu_scene_create(sceneName)
    return setLocalSceneProps(id, sceneName)
end

--- Loads a scene from a given file and makes it the active scene.
--- The file can be a 3d mesh file like .gltf, .fbx, .obj etc.. or a .lunam file.
--- Importing an external file which is not .lunam will take much longer, because conversion is required.
--- @param file: string, path to the file to load
--- @param import_scale: number, scale factor to apply to the imported scene. Only applies if the scene is not a .lunam file
--- @param import_flags: number, flags to control the import process. Only applies if the scene is not a .lunam file
function scene.load(file, import_scale, import_flags)
    if not file or type(file) ~= 'string' then
        eprint('scene name or source file must be provided')
        return false
    end
    if not import_scale or type(import_scale) ~= 'number' or import_scale == 0 then
        import_scale = 1
    end
    if not import_flags or type(import_flags) ~= 'number' then
        import_flags = SCENE_IMPORT_FLAGS.DEFAULT_FLAGS
    end
    if not lfs.attributes(file) then
        eprint(string.format('scene file does not exist: %s', file))
        return false
    end
    local sceneName = file:match("^.+/(.+)$"):match("(.+)%..+") -- extract scene name from file path
    local id = C.__lu_scene_import(sceneName, file, import_scale, band(import_flags, 0xffffffff)) -- create native scene
    return setLocalSceneProps(id, sceneName)
end

-- Do NOT remove this
-- Create a default scene - Lunam requires to always have an active scene
if not scene.new('Untitled') then
    panic('failed to create default scene')
end

return scene
