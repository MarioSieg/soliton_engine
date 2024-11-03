----------------------------------------------------------------------------
-- Lunam Engine scene Module
--
-- Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
------------------------------------------------------------------------------

local ffi = require 'ffi'
local cpp = ffi.C
local bit = require 'bit'
local entity = require 'entity'
local vec3 = require 'vec3'
local quat = require 'quat'
local gmath = require 'gmath'
local time = require 'time'
local debugdraw = require 'debugdraw'
local assets = require 'assets'

local bor, band = bit.bor, bit.band
local rad, sin, cos, tan, atan2, asin, pi = math.rad, math.sin, math.cos, math.tan, math.atan2, math.asin, math.pi

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
    void __lu_scene_set_sun_dir(lua_vec3 sun_dir);
]]

-- Simplified time cycle system:
-- 24 hour day
-- 30 days per month (12 months)
-- 30 * 12 = 360 days per year
local time_cycle = {
    seasons = {
        spring = 1,
        summer = 2,
        autumn = 3,
        winter = 4
    },
    months = {
        january = 1,
        february = 2,
        march = 3,
        april = 4,
        may = 5,
        june = 6,
        july = 7,
        august = 8,
        september = 9,
        october = 10,
        november = 11,
        december = 12
    },
    freeze_time = false,
    freeze_date = false,
    debug_draw = true,
    time_cycle_scale = 1.0, -- Time cycle speed multiplier
    date = {
        day = 29,
        month = 12,
        year = 2024,
        time = 12,
    },
    current_season = 1,
    _north_dir = vec3.unit_x,
    _sun_dir = -vec3.unit_y,
    _sun_dir_quat = quat.identity,
    _latitude = 50.0,
    _delta = 0.0,
    _ecliptic_obliquity = rad(23.44),
}

function time_cycle:get_date_string()
    return string.format('%02d.%02d.%04d', self.date.day, self.date.month, self.date.year)
end

function time_cycle:get_time_string()
    return string.format('%02d:%02d', math.floor(self.date.time), (self.date.time % 1) * 60)
end

function time_cycle:get_date_time_string()
    return string.format('%s %s', self:get_date_string(), self:get_time_string())
end

function time_cycle:get_season_string()
    if self.current_season == self.seasons.spring then
        return 'Spring'
    elseif self.current_season == self.seasons.summer then
        return 'Summer'
    elseif self.current_season == self.seasons.autumn then
        return 'Autumn'
    else
        return 'Winter'
    end
end

function time_cycle:is_hour_passed(hour)
    return self.date.time >= hour
end

function time_cycle:is_day_passed(day)
    return self.date.day >= day
end

function time_cycle:is_month_passed(month)
    return self.date.month >= month
end

function time_cycle:is_year_passed(year)
    return self.date.year >= year
end

function time_cycle:update_with_time(time_hour_24)
    self:_compute_sun_orbit()
    self:_compute_sun_dir(gmath.clamp(time_hour_24, 0, 24) - 12)
end

function time_cycle:_advance()
    if self.debug_draw then
        debugdraw.draw_arrow_dir(vec3.zero, -self._sun_dir * 5.0, colors.yellow)
    end
    if not self.freeze_time then
        self.date.time = self.date.time + time.delta_time * self.time_cycle_scale
        if self.freeze_date then
            self.date.time = self.date.time % 24.0 -- Wrap time around 24 hour day if date is frozen
        end
        self:update_with_time(self.date.time)
    end
    if not self.freeze_date then
        self:_advance_date()
    end
end

function time_cycle:_advance_date()
    if self.date.time >= 24.0 then -- Day has passed
        self.date.time = 0.0
        self.date.day = self.date.day + 1
        if self.date.day > 30 then -- Month has passed
            self.date.day = 1
            self.date.month = self.date.month + 1
            if self.date.month > 12 then -- Year has passed
                self.date.month = 1
                self.date.year = self.date.year + 1
                self:_update_season()
            end
        end
    end
end

function time_cycle:_update_season()
    if self.date.month >= 3 and self.date.month <= 5 then
        self.current_season = self.seasons.spring
    elseif self.date.month >= 6 and self.date.month <= 8 then
        self.current_season = self.seasons.summer
    elseif self.date.month >= 9 and self.date.month <= 11 then
        self.current_season = self.seasons.autumn
    else
        self.current_season = self.seasons.winter
    end
end

function time_cycle:_compute_sun_orbit()
    local day = 30 * self.date.month - 1 + 15
    local lambda = rad(280.46 + 0.9856474 * day) -- Ecliptic longitude
    self._delta = asin(sin(self._ecliptic_obliquity) * sin(lambda))
end

function time_cycle:_compute_sun_dir(hour)
    local latitude = rad(self._latitude)
    local latitude_cos = cos(latitude)
    local latitude_sin = sin(latitude)
    local hh = hour * pi / 12
    local hh_cos = cos(hh)
    local azimuth = atan2(
        sin(hh),
        hh_cos * latitude_sin - tan(self._delta) * latitude_cos
    )
    local altitude = asin(
        latitude_sin * sin(self._delta) + latitude_cos * cos(self._delta) * hh_cos
    )
    local rot0 = quat.from_axis_angle(vec3.up, azimuth)
    local dir = self._north_dir * rot0
    local udx = vec3.cross(vec3.up, dir)
    local rot1 = quat.from_axis_angle(udx, altitude)
    self._sun_dir_quat = rot0 * rot1
    self._sun_dir = dir * rot1
end

local function bit_flag_union(flags)
    local result = 0
    for i = 1, #flags do
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

local scene = {
    name = 'Default',
    id = 0,
    time_cycle = time_cycle
}

function scene._update()
    scene.time_cycle:_advance()
    cpp.__lu_scene_set_sun_dir(scene.time_cycle._sun_dir)
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
--- The file can be a 3d mesh file like .gltf, .fbx, .obj etc.. or a .lunam file.
--- Importing an external file which is not .lunam will take much longer, because conversion is required.
--- @param file: string, path to the file to load
--- @param import_scale: number, scale factor to apply to the imported scene. Only applies if the scene is not a .lunam file
--- @param import_flags: number, flags to control the import process. Only applies if the scene is not a .lunam file
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
    local name = file:match("^.+/(.+)$"):match("(.+)%..+") -- extract scene name from file path
    local id = cpp.__lu_scene_import(name, file, band(import_flags, 0xffffffff)) -- create native scene
    setup_scene_class(id, name)
end

return scene
