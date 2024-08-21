-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local components = require 'components'
local icons = require 'imgui.icons'
local ui = require 'imgui.imgui'

local utils = {}

-- Components that can be added to entities sorted by category
utils.editor_components = {
    [icons.i_toolbox .. ' Core'] = {
        [components.transform._id] = {
            full_name = icons.i_arrows_alt .. ' Transform',
            component = components.transform,
        },
        [components.camera._id] = {
            full_name = icons.i_camera .. ' Camera',
            component = components.camera
        },
        [components.camera._id + 1] = {
            full_name = icons.i_code .. ' Script',
            component = components.camera
        }
    },
    [icons.i_paint_brush .. ' Rendering'] = {
        [components.mesh_renderer._id] = {
            full_name = icons.i_cube .. ' Mesh Renderer',
            component = components.mesh_renderer
        },
        [components.mesh_renderer._id + 1] = {
            full_name = icons.i_lightbulb_on .. ' Light',
            component = components.mesh_renderer
        },
        [components.mesh_renderer._id + 2] = {
            full_name = icons.i_star_shooting .. ' Particle System',
            component = components.mesh_renderer
        },
        [components.mesh_renderer._id + 3] = {
            full_name = icons.i_skeleton .. ' Animator',
            component = components.mesh_renderer
        },
    },
    [icons.i_basketball_ball .. ' Physics'] = {
        [components.character_controller._id] = {
            full_name = icons.i_person_sign .. ' Character Controller',
            component = components.character_controller
        },
        [components.character_controller._id + 1] = {
            full_name = icons.i_box_fragile .. ' Collider',
            component = components.character_controller
        },
        [components.character_controller._id + 2] = {
            full_name = icons.i_truck_moving .. ' Rigid Body',
            component = components.character_controller
        }
    }
}

utils.default_window_size = ui.ImVec2(800, 600)

utils.popupid_new_project = 1
utils.popupid_add_component = 2

local function set_identity(list)
    local set = {}
    for _, l in ipairs(list) do
        set[l] = true
    end
    return set
end

utils.mesh_file_exts = {
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
}
utils.mesh_file_exts = set_identity(utils.mesh_file_exts)

utils.texture_file_exts = {
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
    'webp'
}
utils.texture_file_exts = set_identity(utils.texture_file_exts)

utils.script_file_exts = {
    'lua'
}
utils.script_file_exts = set_identity(utils.script_file_exts)

utils.sound_file_exts = {
    'wav',
    'mp3',
    'ogg',
    'flac',
    'aiff',
    'wma'
}
utils.sound_file_exts = set_identity(utils.sound_file_exts)

utils.font_file_exts = {
    'ttf',
    'otf',
    'woff',
    'woff2'
}
utils.font_file_exts = set_identity(utils.font_file_exts)

utils.material_file_exts = {
    'mat'
}
utils.material_file_exts = set_identity(utils.material_file_exts)

utils.icons_file_exts = {
    'ico',
    'icns'
}
utils.icons_file_exts = set_identity(utils.icons_file_exts)

utils.xaml_file_exts = {
    'xaml'
}
utils.xaml_file_exts = set_identity(utils.xaml_file_exts)

utils.asset_type = {
    DIR = 0,
    MESH = 1,
    TEXTURE = 2,
    SCRIPT = 3,
    FONT = 4,
    MATERIAL = 5,
    SOUND = 6,
    ICON = 7,
    UI_XAML_SHEET = 8,
    UNKNOWN = 9
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
    [utils.asset_type.UI_XAML_SHEET] = 'ui XAML Sheet',
    [utils.asset_type.UNKNOWN] = 'Unknown'
}

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
    return utils.asset_type.UNKNOWN
end

function utils.build_filter_string(items)
    local r = ''
    for k, _ in pairs(items) do r = r .. k .. ',' end
    return r
end

return utils
