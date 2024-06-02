-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'
local inspect = require 'ext.inspect'

local UI = require 'editor.imgui'
local icons = require 'editor.icons'

local AssetExplorer = {
    name = icons.i_folder_tree .. ' Asset View',
    is_visible = ffi.new('bool[1]', true),
    scanDir = 'assets',
    assetList = {},
    dirTree = {},
    columns = ffi.new('int[1]', 10),
    columsRange = { min = 4, max = 15 }
}

function AssetExplorer:buildDirTreeRecursive(path, parent)
    path = path or self.scanDir
    parent = parent or self.dirTree
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = path .. '/' .. entry
            local attr = lfs.attributes(fullPath)
            if attr and (attr.mode == 'directory' or attr.mode == 'file') then
                local node = {
                    name = entry,
                    is_file = attr.mode == 'file',
                    children = {}
                }
                table.insert(parent, node)
                if not node.is_file then
                    self:buildDirTreeRecursive(fullPath, node.children)
                end
            end
        end
    end
end

function AssetExplorer:renderTree()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    local DIR_COLOR = 0xffaaaaaa
    local FILE_COLOR = 0xffcccccc
    UI.PushStyleColor(ffi.C.ImGuiCol_Text, DIR_COLOR)
    if UI.BeginChild('AssetTree') then
        for i = 1, #self.dirTree do
            local node = self.dirTree[i]
            if UI.TreeNode(node.name) then
                for j=1, #node.children do
                    local child = node.children[j]
                    if child.is_file then
                        UI.PushStyleColor(ffi.C.ImGuiCol_Text, FILE_COLOR)
                        UI.Indent()
                        UI.TextUnformatted(child.name)
                        UI.Unindent()
                        UI.PopStyleColor()
                    else
                        if UI.TreeNode(child.name) then
                            UI.TreePop()
                        end
                    end
                end
                UI.TreePop()
            end
        end
    end
    UI.EndChild()
    UI.PopStyleColor()
end

function AssetExplorer:expandAssetListRecursive(path) -- TODO: Only do for top directory and expand on demand
    path = path or self.scanDir
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = path .. '/' .. entry
            local attr = lfs.attributes(fullPath)
            if attr.mode == 'directory' then
                self:expandAssetListRecursive(fullPath)
            else
                local fileExt = string.match(entry, '[^.]+$')
                if not fileExt then
                    fileExt = 'unknown'
                end
                
                table.insert(self.assetList, {
                    path = fullPath,
                    name = entry,
                    size = attr.size,
                    date = attr.modification,
                    type = determineAssetType(fileExt)
                })
            end
        end
    end
end

function AssetExplorer:buildAssetList()
    self.assetList = {}
    if not lfs.attributes(self.scanDir) then
        eprint('AssetExplorer failed to scan directory: ' .. self.scanDir)
        return
    end
    self:expandAssetListRecursive()
end

function AssetExplorer:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.is_visible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if UI.BeginMenuBar() then
            if UI.SmallButton(icons.i_redo_alt .. ' Refresh') then
                
            end
            UI.PushItemWidth(75)
            UI.SliderInt('##AssetExplorerColumns', self.columns, self.columsRange.min, self.columsRange.max, '%d Cols')
            UI.PopItemWidth()
            UI.EndMenuBar()
        end
        self:renderTree()
        local win_size = UI.GetWindowSize()
        local cols = self.columns[0]
        local tile = (win_size.x / cols) - 32
        local grid_size = UI.ImVec2(tile, tile)
        if UI.BeginChild('AssetScrollingRegion', UI.ImVec2(0, -UI.GetFrameHeightWithSpacing()), false, 0) then
            UI.Columns(cols, 'AssetColumns', true)
            local r = math.random()
            for i = 1, #self.assetList do
                local asset = self.assetList[i]
                local label = asset.name
                UI.PushID(i * r)
                -- Render icon with file name in a grid:
                if UI.Button(label, grid_size) then
                    print('Clicked on asset: ' .. label)
                end
                if UI.IsItemHovered() then
                    UI.SetTooltip(label .. ' - ' .. ASSET_TYPE_NAMES[asset.type])
                end
                UI.PopID()
                UI.NextColumn()
            end
            UI.EndChild()
        end
    end
    UI.End()
end

AssetExplorer:buildDirTreeRecursive() -- Build directory tree once on start
AssetExplorer:buildAssetList() -- Build asset list once on start

return AssetExplorer
