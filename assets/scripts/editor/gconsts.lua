-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local UI = require 'editor.imgui'

default_window_size = UI.ImVec2(800, 600)

local function set_identity(list)
    local set = {}
    for _, l in ipairs(list) do
        set[l] = true
    end
    return set
end

mesh_file_exts = {
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
mesh_file_exts = set_identity(mesh_file_exts)

texture_file_exts = {
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
texture_file_exts = set_identity(texture_file_exts)

script_file_exts = {
    'lua'
}
script_file_exts = set_identity(script_file_exts)

sound_file_exts = {
    'wav',
    'mp3',
    'ogg',
    'flac',
    'aiff',
    'wma'
}
sound_file_exts = set_identity(sound_file_exts)

font_file_exts = {
    'ttf',
    'otf',
    'woff',
    'woff2'
}
font_file_exts = set_identity(font_file_exts)

material_file_exts = {
    'mat'
}
material_file_exts = set_identity(material_file_exts)

icons_file_exts = {
    'ico',
    'icns'
}
icons_file_exts = set_identity(icons_file_exts)

xaml_file_exts = {
    'xaml'
}
xaml_file_exts = set_identity(xaml_file_exts)

asset_type = {
    MESH = 0,
    TEXTURE = 1,
    SCRIPT = 2,
    FONT = 3,
    MATERIAL = 4,
    SOUND = 5,
    ICON = 6,
    UI_XAML_SHEET = 7,
    UNKNOWN = 8
}
asset_type_names = {
    [asset_type.MESH] = 'Mesh',
    [asset_type.TEXTURE] = 'Texture',
    [asset_type.SCRIPT] = 'Lua Script',
    [asset_type.FONT] = 'Font',
    [asset_type.MATERIAL] = 'Material',
    [asset_type.SOUND] = 'Sound',
    [asset_type.ICON] = 'Icon',
    [asset_type.UI_XAML_SHEET] = 'UI XAML Sheet',
    [asset_type.UNKNOWN] = 'Unknown'
}

function determine_asset_type(ext)
    if texture_file_exts[ext] then return asset_type.TEXTURE end
    if mesh_file_exts[ext] then return asset_type.MESH end
    if script_file_exts[ext] then return asset_type.SCRIPT end
    if font_file_exts[ext] then return asset_type.FONT end
    if material_file_exts[ext] then return asset_type.MATERIAL end
    if sound_file_exts[ext] then return asset_type.SOUND end
    if icons_file_exts[ext] then return asset_type.ICON end
    if xaml_file_exts[ext] then return asset_type.UI_XAML_SHEET end
    return asset_type.UNKNOWN
end

function build_filter_string(items)
    local r = ''
    for k, _ in pairs(items) do r = r .. k .. ',' end
    return r
end
