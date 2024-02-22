-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local C = ffi.C
local bit = require 'bit'
local bor = bit.bor
local band = bit.band
local Entity = require 'lu.entity'
local Components = require 'lu.components'

ffi.cdef[[
    uint32_t __lu_scene_import(const char* name, const char* model_file, double scale, uint32_t load_flags);
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
]]

local function mergeFlags(flags)
    local result = 0
    for i=1, #flags do
        result = bor(result, flags[i])
    end
    assert(result <= 0xffffffff)
    return result
end

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

SCENE_IMPORT_FLAGS.PRESET_REALTIME_QUALITY = mergeFlags({
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
    SCENE_IMPORT_FLAGS.OPTIMIZE_GRAPH
})

SCENE_IMPORT_FLAGS.PRESET_CONVERT_TO_LH = mergeFlags({
    SCENE_IMPORT_FLAGS.MAKE_LEFT_HANDED,
    SCENE_IMPORT_FLAGS.FLIP_UVS,
    SCENE_IMPORT_FLAGS.FLIP_WINDING_ORDER
})

SCENE_IMPORT_FLAGS.DEFAULT_FLAGS = mergeFlags({
    SCENE_IMPORT_FLAGS.PRESET_REALTIME_QUALITY ,
    SCENE_IMPORT_FLAGS.PRESET_CONVERT_TO_LH
})

local Scene = {
    name = 'Untitled',
    id = 0
}

function Scene.__onStart()
    C.__lu_scene_start()
end

function Scene.__onTick()
    C.__lu_scene_tick()
end

function Scene.spawn(name)
    return Entity:fromId(C.__lu_scene_spawn_entity(name))
end

function Scene.despawn(entity)
    C.__lu_scene_despawn_entity(entity.id)
    entity.id = nil
end

function Scene.getEntityByName(name)
    return Entity:fromId(C.__lu_scene_get_entity_by_name(name))
end

function Scene.fullEntityQueryStart()
    C.__lu_scene_full_entity_query_start()
end

function Scene.fullEntityQueryNextTable()
    return C.__lu_scene_full_entity_query_next_table()
end

function Scene.fullEntityQueryGet(i)
    return Entity:fromId(C.__lu_scene_full_entity_query_get(i))
end

function Scene.fullEntityQueryEnd()
    C.__lu_scene_full_entity_query_end()
end

function Scene.setActiveCameraEntity(entity)
    assert(entity:hasComponent(Components.Transform) and entity:hasComponent(Components.Camera))
    C.__lu_scene_set_active_camera_entity(entity.id)
end

function Scene.load(scene_name, file, scale, flags)
    scale = scale or 1.0
    flags = flags or SCENE_IMPORT_FLAGS.DEFAULT_FLAGS
    assert(scene_name or file, 'scene name or gltf file must be provided')
    if file and type(file) == 'string' then
        if not lfs.attributes(file) then
            panic(string.format('gltf file %s does not exist', file))
        end
        if not scene_name then
            scene_name = file:match("^.+/(.+)$")
            scene_name = scene_name:match("(.+)%..+")
        end
    else
        scene_name = scene_name or 'untitled'
        file = ''
    end
    Scene.name = scene_name
    assert(type(scene_name) == 'string')
    local id = C.__lu_scene_import(scene_name, file, scale, band(flags, 0xffffffff)) -- create native scene
    assert(type(id) == 'number' and id ~= 0, 'failed to create scene')
    Scene.id = id
    Scene.__onStart() -- invoke start hook
    print(string.format('Created new scene: %s, id: %x', scene_name, id))
    collectgarbage('collect') -- collect garbage after new scene is created
    collectgarbage('stop')
end

Scene.load('Default', nil)

return Scene
