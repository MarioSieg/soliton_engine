-- Copyright (c) 2024 Mario 'Neo' Sieg. All Rights Reserved.

local bor, band = bit.bor, bit.band

local function bit_flag_union(flags)
    local result = 0
    for i = 1, #flags do
        result = bor(result, flags[i])
    end
    return band(result, 0xffffffff)
end

--- scene import flags for post processing.
--- Only applies if the scene is not a .lunam file.
local scene_import_flags = {
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

scene_import_flags.preset_realtime_quality = bit_flag_union({
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

scene_import_flags.preset_convert_to_lh = bit_flag_union({
    scene_import_flags.make_left_handed,
    scene_import_flags.flip_uvs,
    scene_import_flags.flip_winding_order
})

scene_import_flags.default = bit_flag_union({
    scene_import_flags.preset_realtime_quality,
    scene_import_flags.preset_convert_to_lh
})

return scene_import_flags
