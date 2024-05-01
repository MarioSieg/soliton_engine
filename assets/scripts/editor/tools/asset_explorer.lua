-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local inspect = require 'ext.inspect'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require('Time')
local Scene = require('Scene')
local EFLAGS = ENTITY_FLAGS

local AssetExplorer = {
    name = ICONS.FOLDER_TREE..' Asset View',
    isVisible = ffi.new('bool[1]', true),
    scanDir = 'assets',
    assetList = {},
    columns = ffi.new('int[1]', 10)
}

function AssetExplorer:expandAssetListRecursive(path) -- TODO: Only do for top directory and expand on demand
    path = path or self.scanDir
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = path..'/'..entry
            local attr = lfs.attributes(fullPath)
            if attr.mode == 'directory' then
                self:expandAssetListRecursive(fullPath)
            else
                table.insert(self.assetList, {
                    path = fullPath,
                    name = entry,
                    size = attr.size,
                    date = attr.modification
                })
            end
        end
    end
end

function AssetExplorer:buildAssetList()
    self.assetList = {}
    if not lfs.attributes(self.scanDir) then
        perror('AssetExplorer failed to scan directory: '..self.scanDir)
        return
    end
    self:expandAssetListRecursive()
end

AssetExplorer:buildAssetList() -- Build asset list once on start

local WINDOW_FLAGS = 0

function AssetExplorer:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.isVisible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if UI.BeginMenuBar() then
            if UI.SmallButton(ICONS.REDO_ALT..' Refresh') then
                
            end
            UI.DragInt('##AssetExplorerColumns', self.columns, 1, 2, 32, '%d columns')
            UI.EndMenuBar()
        end
        local win_size = UI.GetWindowSize()
        local cols = self.columns[0]
        local tile = (win_size.x/cols)-32
        local grid_size = UI.ImVec2(tile, tile)
        if UI.BeginChild('AssetScrollingRegion', UI.ImVec2(0, -UI.GetFrameHeightWithSpacing()), false, WINDOW_FLAGS) then
            UI.Columns(cols, 'AssetColumns', true)
            for i=1, #self.assetList do
                local asset = self.assetList[i]
                local label = asset.name
                -- Render icon with file name in a grid:
                if UI.Button(label, grid_size) then
                    print('Clicked on asset: '..label)
                end
                UI.NextColumn()
            end
            UI.EndChild()
        end
    end
    UI.End()
end

return AssetExplorer
