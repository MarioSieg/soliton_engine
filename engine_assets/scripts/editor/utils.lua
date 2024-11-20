-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local icons = require 'imgui.icons'
local ui = require 'imgui.imgui'
local c_transform = require 'components.transform'
local c_camera = require 'components.camera'
local c_character_controller = require 'components.character_controller'
local c_mesh_renderer = require 'components.mesh_renderer'

local function new_identity_map(list)
    local set = {}
    for _, l in ipairs(list) do
        set[l] = true
    end
    return set
end

local function map_len(T)
    local count = 0
    for _ in pairs(T) do count = count + 1 end
    return count
end

local _enumerator = 0
local function next_enum()
    _enumerator = _enumerator + 1
    return _enumerator
end

local utils = {}

-- Components that can be added to entities sorted by category
utils.editor_components = {
    [icons.i_toolbox .. ' Core'] = {
        [c_transform._id] = {
            full_name = icons.i_arrows_alt .. ' Transform',
            component = c_transform,
        },
        [c_camera._id] = {
            full_name = icons.i_camera .. ' Camera',
            component = c_camera
        },
        [c_camera._id + 1] = {
            full_name = icons.i_code .. ' Script',
            component = c_camera
        }
    },
    [icons.i_paint_brush .. ' Rendering'] = {
        [c_mesh_renderer._id] = {
            full_name = icons.i_cube .. ' Mesh Renderer',
            component = c_mesh_renderer
        },
        [c_mesh_renderer._id + 1] = {
            full_name = icons.i_lightbulb_on .. ' Light',
            component = c_mesh_renderer
        },
        [c_mesh_renderer._id + 2] = {
            full_name = icons.i_star_shooting .. ' Particle System',
            component = c_mesh_renderer
        },
        [c_mesh_renderer._id + 3] = {
            full_name = icons.i_skeleton .. ' Animator',
            component = c_mesh_renderer
        },
    },
    [icons.i_basketball_ball .. ' Physics'] = {
        [c_character_controller._id] = {
            full_name = icons.i_person_sign .. ' Character Controller',
            component = c_character_controller
        },
        [c_character_controller._id + 1] = {
            full_name = icons.i_box_fragile .. ' Collider',
            component = c_character_controller
        },
        [c_character_controller._id + 2] = {
            full_name = icons.i_truck_moving .. ' Rigid Body',
            component = c_character_controller
        }
    }
}

utils.default_window_size = ui.ImVec2(800, 600)

utils.popupid_new_project = 1
utils.popupid_add_component = 2

utils.mesh_file_exts = new_identity_map({
    '3d',
    '3ds',
    '3mf',
    'ac',
    'ac3d',
    'acc',
    'amj',
    'ase',
    'ask',
    'b3d',
    'bvh',
    'csm',
    'cob',
    'dae',
    'dxf',
    'enff',
    'fbx',
    'gltf',
    'glb',
    'hmb',
    'ifc',
    'irr',
    'lwo',
    'lws',
    'lxo',
    'm3d',
    'md2',
    'md3',
    'md5',
    'mdc',
    'mdl',
    'mesh',
    'mot',
    'ms3d',
    'ndo',
    'nff',
    'obj',
    'off',
    'ogex',
    'ply',
    'pmx',
    'prj',
    'q3o',
    'q3s',
    'raw',
    'scn',
    'sib',
    'smd',
    'stp',
    'stl',
    'ter',
    'uc',
    'vta',
    'x',
    'x3d',
    'xgl',
    'zgl'
})

utils.texture_file_exts = new_identity_map({
    'bmp',
    'dds',
    'exr',
    'hdr',
    'jpg',
    'jpeg',
    'png',
    'psd',
    'tga',
    'tiff',
    'webp',
    'ktx'
})

utils.script_file_exts = new_identity_map({
    'lua'
})

utils.sound_file_exts = new_identity_map({
    'wav',
    'mp3',
    'ogg',
    'flac',
    'aiff',
    'wma'
})

utils.font_file_exts = new_identity_map({
    'ttf',
    'otf',
    'woff',
    'woff2'
})

utils.material_file_exts = new_identity_map({
    'mat'
})

utils.icons_file_exts = new_identity_map({
    'ico',
    'icns'
})

utils.xaml_file_exts = new_identity_map({
    'xaml'
})

utils.shader_file_exts = new_identity_map({
    'h', -- We assume that the header file is a GLSL shader header file
    'frag',
    'vert',
    'geom',
    'tesc',
    'tese',
    'comp',
    'rgen',
    'rint',
    'rahit',
    'rchit',
    'rmiss',
    'rcall',
    'rclose',
    'anyhit',
    'closesthit',
    'intersection',
    'miss',
    'callable',
    'mesh'
})

utils.text_file_exts = new_identity_map({
    'txt',
    'md',
    'xml',
    'json',
    'csv',
    'yml',
    'yaml',
    'ini',
    'cfg',
    'conf',
    'log',
    'bat',
    'sh'
})

utils.binary_file_exts = new_identity_map({
    'bin',
    'dat',
    'db',
    'blob',
    'zip',
    'lupack',
    'pak'
})

utils.asset_type = {
    DIR = next_enum(),
    MESH = next_enum(),
    TEXTURE = next_enum(),
    SCRIPT = next_enum(),
    FONT = next_enum(),
    MATERIAL = next_enum(),
    SOUND = next_enum(),
    ICON = next_enum(),
    UI_XAML_SHEET = next_enum(),
    SHADER = next_enum(),
    TEXT_FILE = next_enum(),
    BINARY_FILE = next_enum(),
    UNKNOWN = next_enum()
}

utils.asset_type_names = {
    [utils.asset_type.DIR] = 'Directory',
    [utils.asset_type.MESH] = 'Mesh',
    [utils.asset_type.TEXTURE] = 'Texture',
    [utils.asset_type.SCRIPT] = 'Lua Script',
    [utils.asset_type.FONT] = 'Font',
    [utils.asset_type.MATERIAL] = 'Material',
    [utils.asset_type.SOUND] = 'Sound',
    [utils.asset_type.ICON] = 'Icon',
    [utils.asset_type.UI_XAML_SHEET] = 'UI XAML Sheet',
    [utils.asset_type.SHADER] = 'GLSL Shader',
    [utils.asset_type.TEXT_FILE] = 'Text Data File',
    [utils.asset_type.BINARY_FILE] = 'Binary Data File',
    [utils.asset_type.UNKNOWN] = 'Unknown'
}
assert(#utils.asset_type_names == map_len(utils.asset_type))

utils.asset_type_icons = {
    [utils.asset_type.DIR] = icons.i_folder,
    [utils.asset_type.MESH] = icons.i_cube,
    [utils.asset_type.TEXTURE] = icons.i_image,
    [utils.asset_type.SCRIPT] = icons.i_code,
    [utils.asset_type.FONT] = icons.i_font,
    [utils.asset_type.MATERIAL] = icons.i_paint_brush,
    [utils.asset_type.SOUND] = icons.i_volume_up,
    [utils.asset_type.ICON] = icons.i_icons,
    [utils.asset_type.UI_XAML_SHEET] = icons.i_window,
    [utils.asset_type.SHADER] = icons.i_fan,
    [utils.asset_type.TEXT_FILE] = icons.i_text,
    [utils.asset_type.BINARY_FILE] = icons.i_database,
    [utils.asset_type.UNKNOWN] = icons.i_question
}
assert(#utils.asset_type_icons == map_len(utils.asset_type))

utils.asset_type_colors = {
    [utils.asset_type.DIR] = 0xffffe4e4,       -- Light Pink: Folder/Directory
    [utils.asset_type.MESH] = 0xffadd8ff,      -- Pastel Blue: Mesh
    [utils.asset_type.TEXTURE] = 0xffffe7a9,   -- Light Yellow: Texture
    [utils.asset_type.SCRIPT] = 0xffbaffc9,    -- Pastel Green: Script
    [utils.asset_type.FONT] = 0xffd8b9ff,      -- Light Purple: Font
    [utils.asset_type.MATERIAL] = 0xffdab6ff,  -- Pastel Violet: Material
    [utils.asset_type.SOUND] = 0xffffb3b3,     -- Pastel Red: Sound
    [utils.asset_type.ICON] = 0xffffe5a9,      -- Light Gold: Icon
    [utils.asset_type.UI_XAML_SHEET] = 0xffa9ffff, -- Pastel Cyan: UI/XAML Sheet
    [utils.asset_type.SHADER] = 0xffb3a9ff,    -- Light Indigo: Shader
    [utils.asset_type.TEXT_FILE] = 0xffe5e5e5, -- Light Grey: Text File
    [utils.asset_type.BINARY_FILE] = 0xffe5e5e5, -- Light Grey: Binary File
    [utils.asset_type.UNKNOWN] = 0xffcccccc    -- Light Grey: Unknown
}
assert(#utils.asset_type_colors == map_len(utils.asset_type))

function utils.determine_asset_type(ext)
    if ext == nil then return utils.asset_type.DIR end
    if utils.texture_file_exts[ext] then return utils.asset_type.TEXTURE end
    if utils.mesh_file_exts[ext] then return utils.asset_type.MESH end
    if utils.script_file_exts[ext] then return utils.asset_type.SCRIPT end
    if utils.font_file_exts[ext] then return utils.asset_type.FONT end
    if utils.material_file_exts[ext] then return utils.asset_type.MATERIAL end
    if utils.sound_file_exts[ext] then return utils.asset_type.SOUND end
    if utils.icons_file_exts[ext] then return utils.asset_type.ICON end
    if utils.xaml_file_exts[ext] then return utils.asset_type.UI_XAML_SHEET end
    if utils.shader_file_exts[ext] then return utils.asset_type.SHADER end
    if utils.text_file_exts[ext] then return utils.asset_type.TEXT_FILE end
    if utils.binary_file_exts[ext] then return utils.asset_type.BINARY_FILE end
    return utils.asset_type.UNKNOWN
end

function utils.prettify_name(name, lim)
    lim = lim or 20
    local pretty_name = name
    if string.len(name) > lim then
        pretty_name = string.sub(name, 1, lim) .. '...'
    end
    return pretty_name
end

function utils.build_filter_string(items)
    local r = ''
    for k, _ in pairs(items) do r = r .. k .. ',' end
    return r
end

return utils
