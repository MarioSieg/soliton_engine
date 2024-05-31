-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local UI = require 'editor.imgui'

WINDOW_SIZE = UI.ImVec2(800, 600)

local function set_identity(list)
    local set = {}
    for _, l in ipairs(list) do
        set[l] = true
    end
    return set
end

MESH_FILE_EXTS = {
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
MESH_FILE_EXTS = set_identity(MESH_FILE_EXTS)

TEXTURE_FILE_EXTS = {
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
TEXTURE_FILE_EXTS = set_identity(TEXTURE_FILE_EXTS)

SCRIPT_FILE_EXTS = {
    'lua'
}
SCRIPT_FILE_EXTS = set_identity(SCRIPT_FILE_EXTS)

SOUND_FILE_EXTS = {
    'wav',
    'mp3',
    'ogg',
    'flac',
    'aiff',
    'wma'
}
SOUND_FILE_EXTS = set_identity(SOUND_FILE_EXTS)

FONT_FILE_EXTS = {
    'ttf',
    'otf',
    'woff',
    'woff2'
}
FONT_FILE_EXTS = set_identity(FONT_FILE_EXTS)

MATERIAL_FILE_EXTS = {
    'mat'
}
MATERIAL_FILE_EXTS = set_identity(MATERIAL_FILE_EXTS)

ICONS_FILE_EXTS = {
    'ico',
    'icns'
}
ICONS_FILE_EXTS = set_identity(ICONS_FILE_EXTS)

XAML_FILE_EXTS = {
    'xaml'
}
XAML_FILE_EXTS = set_identity(XAML_FILE_EXTS)

ASSET_TYPE = {
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
ASSET_TYPE_NAMES = {
    [ASSET_TYPE.MESH] = 'Mesh',
    [ASSET_TYPE.TEXTURE] = 'Texture',
    [ASSET_TYPE.SCRIPT] = 'Lua Script',
    [ASSET_TYPE.FONT] = 'Font',
    [ASSET_TYPE.MATERIAL] = 'Material',
    [ASSET_TYPE.SOUND] = 'Sound',
    [ASSET_TYPE.ICON] = 'Icon',
    [ASSET_TYPE.UI_XAML_SHEET] = 'UI XAML Sheet',
    [ASSET_TYPE.UNKNOWN] = 'Unknown'
}

function determineAssetType(ext)
    if TEXTURE_FILE_EXTS[ext] then return ASSET_TYPE.TEXTURE end
    if MESH_FILE_EXTS[ext] then return ASSET_TYPE.MESH end
    if SCRIPT_FILE_EXTS[ext] then return ASSET_TYPE.SCRIPT end
    if FONT_FILE_EXTS[ext] then return ASSET_TYPE.FONT end
    if MATERIAL_FILE_EXTS[ext] then return ASSET_TYPE.MATERIAL end
    if SOUND_FILE_EXTS[ext] then return ASSET_TYPE.SOUND end
    if ICONS_FILE_EXTS[ext] then return ASSET_TYPE.ICON end
    if XAML_FILE_EXTS[ext] then return ASSET_TYPE.UI_XAML_SHEET end
    return ASSET_TYPE.UNKNOWN
end

function build_filter_string(items)
    local r = ''
    for k, _ in pairs(items) do r = r..k..',' end
    return r
end
