----------------------------------------------------------------------------
-- Lunam Engine scene Module
--
-- Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C
local bit = require 'bit'
local bor, band = bit.bor, bit.band
local entity = require 'lu.entity'

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

local function combine_flags(flags)
    local result = 0
    for i=1, #flags do
        result = bor(result, flags[i])
    end
    return band(result, 0xffffffff)
end

--- scene import flags for post processing.
--- Only applies if the scene is not a .lunam file.
scene_import_flags = {
    none = 0x0,
    calc_tangent_space = 0x1,
    join_identical_vertices = 0x2,
    make_left_handed = 0x4,
    triangulate = 0x8,
    remove_component = 0x10,
    gen_normals = 0x20,
    gen_smooth_normals = 0x40,
    split_large_meshes = 0x80,
    pre_transform_vertices = 0x100,
    limit_bone_weights = 0x200,
    validate_data_structure = 0x400,
    improve_cache_locality = 0x800,
    remove_redundant_materials = 0x1000,
    fix_infacing_normals = 0x2000,
    populate_armature_data = 0x4000,
    sort_by_ptype = 0x8000,
    find_degenerates = 0x10000,
    find_invalid_data = 0x20000,
    gen_uv_coords = 0x40000,
    transform_uv_coords = 0x80000,
    find_instances = 0x100000,
    optimize_meshes = 0x200000,
    optimize_graph = 0x400000,
    flip_uvs = 0x800000,
    flip_winding_order = 0x1000000,
    split_by_bone_count = 0x2000000,
    debone = 0x4000000,
    global_scale = 0x8000000,
    embed_textures = 0x10000000,
    force_gen_normals = 0x20000000,
    drop_normals = 0x40000000,
    gen_bounding_boxes = 0x80000000,
}

scene_import_flags.preset_realtime_quality = combine_flags({
    scene_import_flags.calc_tangent_space,
    scene_import_flags.gen_smooth_normals,
    scene_import_flags.join_identical_vertices,
    scene_import_flags.improve_cache_locality,
    scene_import_flags.limit_bone_weights,
    scene_import_flags.remove_redundant_materials,
    scene_import_flags.triangulate,
    scene_import_flags.gen_uv_coords,
    scene_import_flags.sort_by_ptype,
    scene_import_flags.find_degenerates,
    scene_import_flags.find_invalid_data,
    scene_import_flags.find_instances,
    scene_import_flags.optimize_meshes,
    -- scene_import_flags.optimize_graph
})

scene_import_flags.preset_convert_to_lh = combine_flags({
    scene_import_flags.make_left_handed,
    scene_import_flags.flip_uvs,
    scene_import_flags.flip_winding_order
})

scene_import_flags.default = combine_flags({
    scene_import_flags.preset_realtime_quality ,
    scene_import_flags.preset_convert_to_lh
})

local scene = {
    name = 'Untitled',
    id = 0
}

function scene._start()
    cpp.__lu_scene_start()
end

function scene._update()
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

function scene.set_active_camera_entity(entity)
    cpp.__lu_scene_set_active_camera_entity(entity.id)
end

function scene.get_active_camera_entity()
    return entity:from_native_id(cpp.__lu_scene_get_active_camera_entity())
end

local function setup_scene_class(id, name)
    -- Check if the id is valid
    if type(id) ~= 'number' or id == 0 then
        eprint(string.format('Failed to create scene, invalid id returned: %s', name))
        return false
    end

    assert(name and type(name) == 'string')

    -- Make scene active
    scene.name = name
    scene.id = id
    scene._start() -- invoke start hook
    print(string.format('Created new scene: %s, id: %x', name, id))

    -- Perform one full GC cycle to clean up any garbage
    collectgarbage('collect')
    collectgarbage('stop')
    return true
end

--- Creates a new, empty scene with the given name and makes it the active scene.
--- @param name: string, name of the new scene
function scene.new(name)
    name = name or 'Untitled scene'
    local id = cpp.__lu_scene_create(name)
    return setup_scene_class(id, name)
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
        import_flags = scene_import_flags.default
    end
    if not lfs.attributes(file) then
        eprint(string.format('scene file does not exist: %s', file))
        return false
    end
    local name = file:match("^.+/(.+)$"):match("(.+)%..+") -- extract scene name from file path
    local id = cpp.__lu_scene_import(name, file, import_scale, band(import_flags, 0xffffffff)) -- create native scene
    return setup_scene_class(id, name)
end

-- Do NOT remove this
-- Create a default scene - Lunam requires to always have an active scene
if not scene.new('Untitled') then
    panic('failed to create default scene')
end

return scene
