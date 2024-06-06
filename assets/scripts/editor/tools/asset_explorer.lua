-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ui = require 'editor.imgui'
local icons = require 'editor.icons'

local asset_explorer = {
    name = icons.i_folder_tree .. ' Asset View',
    is_visible = ffi.new('bool[1]', true),
    target_scan_dir = 'assets',
    asset_list = {},
    dir_tree = {},
    columns = ffi.new('int[1]', 10),
    columns_range = { min = 4, max = 15 },

    _tree_ratio = 0.2,
    _padding = 8
}

function asset_explorer:build_asset_dir_recursive(path, parent)
    path = path or self.target_scan_dir
    parent = parent or self.dir_tree
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
                    self:build_asset_dir_recursive(fullPath, node.children)
                end
            end
        end
    end
end

function asset_explorer:draw_asset_tree_structure(ratio)
    local DIR_COLOR = 0xffaaaaaa
    local FILE_COLOR = 0xffcccccc
    ui.PushStyleColor(ffi.C.ImGuiCol_Text, DIR_COLOR)
    if ui.BeginChild('AssetTree', ui.ImVec2(ratio * ui.GetWindowSize().x, 0)) then
        for i = 1, #self.dir_tree do
            local node = self.dir_tree[i]
            if ui.TreeNode(node.name) then
                for j=1, #node.children do
                    local child = node.children[j]
                    if child.is_file then
                        ui.PushStyleColor(ffi.C.ImGuiCol_Text, FILE_COLOR)
                        ui.Indent()
                        ui.TextUnformatted(child.name)
                        ui.Unindent()
                        ui.PopStyleColor()
                    else
                        if ui.TreeNode(child.name) then
                            ui.TreePop()
                        end
                    end
                end
                ui.TreePop()
            end
        end
    end
    ui.EndChild()
    ui.PopStyleColor()
end

function asset_explorer:expand_asst_list_recursive_within_dir(path) -- TODO: Only do for top directory and expand on demand
    path = path or self.target_scan_dir
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' then
            local fullPath = path .. '/' .. entry
            local attr = lfs.attributes(fullPath)
            if attr.mode == 'directory' then
                self:expand_asst_list_recursive_within_dir(fullPath)
            else
                local ext = string.match(entry, '[^.]+$')
                if not ext then
                    ext = 'unknown'
                end
                
                table.insert(self.asset_list, {
                    path = fullPath,
                    name = entry,
                    size = attr.size,
                    date = attr.modification,
                    type = determine_asset_type(ext)
                })
            end
        end
    end
end

function asset_explorer:build_asset_list_in_dir()
    self.asset_list = {}
    if not lfs.attributes(self.target_scan_dir) then
        eprint('asset_explorer failed to scan directory: ' .. self.target_scan_dir)
        return
    end
    self:expand_asst_list_recursive_within_dir()
end

function asset_explorer:render()
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if ui.BeginMenuBar() then
            if ui.SmallButton(icons.i_redo_alt .. ' Refresh') then
                
            end
            ui.PushItemWidth(75)
            ui.SliderInt('##AssetExplorerColumns', self.columns, self.columns_range.min, self.columns_range.max, '%d Cols')
            ui.PopItemWidth()
            ui.EndMenuBar()
        end
        local cols = self.columns[0]
        local win_size_x = (1 - self._tree_ratio) * ui.GetWindowSize().x
        local tile = (win_size_x / cols) - self._padding
        local grid_size = ui.ImVec2(tile, tile)
        self:draw_asset_tree_structure(self._tree_ratio)
        ui.SameLine()
        if ui.BeginChild('AssetScrollingRegion', ui.ImVec2(win_size_x, 0), false) then
            ui.Columns(cols, 'AssetColumns', false)
            local r = math.random()
            for i = 1, #self.asset_list do
                local asset = self.asset_list[i]
                local label = asset.name
                ui.PushID(i * r)
                -- Render icon with file name in a grid:
                if ui.Button(label, grid_size) then
                    print('Clicked on asset: ' .. label)
                end
                if ui.IsItemHovered() then
                    ui.SetTooltip(label .. ' - ' .. asset_type_names[asset.type])
                end
                ui.PopID()
                ui.NextColumn()
            end
            ui.EndChild()
        end
    end
    ui.End()
end

asset_explorer:build_asset_dir_recursive() -- Build directory tree once on start
asset_explorer:build_asset_list_in_dir() -- Build asset list once on start

return asset_explorer
