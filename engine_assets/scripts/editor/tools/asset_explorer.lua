-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'

local asset_explorer = {
    name = icons.i_folder_tree .. ' Asset View',
    is_visible = ffi.new('bool[1]', true),
    root_asset_dir = 'engine_assets',
    current_working_dir = '',
    asset_list = {},
    dir_tree = {},
    columns = ffi.new('int[1]', 12),
    columns_range = { min = 4, max = 15 },

    _tree_ratio = 0.2,
    _padding = 8
}

asset_explorer.current_working_dir = asset_explorer.root_asset_dir -- We start at the asset root

function asset_explorer:populate_asset_tree_list_recursive(path, parent)
    path = path or self.root_asset_dir
    parent = parent or self.dir_tree
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' and not entry:match("^%.") then
            local full_path = path .. '/' .. entry
            local attr = lfs.attributes(full_path)
            if attr and (attr.mode == 'directory' or attr.mode == 'file') then
                local node = {
                    name = entry,
                    is_file = attr.mode == 'file',
                    path = full_path,
                    children = {}
                }
                table.insert(parent, node)
                if not node.is_file then
                    self:populate_asset_tree_list_recursive(full_path, node.children)
                end
            end
        end
    end
end

function asset_explorer:build_asset_list_in_working_dir_impl(path)
    for entry in lfs.dir(path) do
        if entry ~= '.' and entry ~= '..' and not path:match("^%.") then
            local full_path = path .. '/' .. entry
            local attr = lfs.attributes(full_path)
            local is_dir = attr.mode == 'directory'
            local ext = nil
            if not is_dir then
                ext = string.match(entry, '[^.]+$')
                if not ext then
                    ext = 'unknown'
                end
            end
            table.insert(self.asset_list, {
                path = full_path,
                name = entry,
                size = is_dir and 0 or attr.size,
                date = is_dir and 0 or attr.modification,
                type = determine_asset_type(ext)
            })
        end
    end
end

function asset_explorer:build_asset_list_in_working_dir()
    self.asset_list = {}
    if not lfs.attributes(self.current_working_dir) then
        eprint('asset_explorer failed to scan directory: ' .. self.current_working_dir)
        return
    end
    self:build_asset_list_in_working_dir_impl(self.current_working_dir)
end

local dir_color = 0xffaaaaaa
local file_color = 0xffcccccc

function asset_explorer:_tree_node_action_recursive(node)
    local flags = node.is_file and ffi.C.ImGuiTreeNodeFlags_Leaf or 0
    local name = node.name
    local node_open = ui.TreeNodeEx(name, flags)
    if ui.IsItemClicked() and not node.is_file then
        print("Selected path: " .. node.path)
        self.current_working_dir = node.path
        self:build_asset_list_in_working_dir()
    end
    if node_open then
        for j = 1, #node.children do
            local child = node.children[j]
            self:_tree_node_action_recursive(child)
        end

        ui.TreePop()
    end
end

function asset_explorer:draw_asset_tree_structure(ratio)
    ui.PushStyleColor(ffi.C.ImGuiCol_Text, dir_color)
    if ui.BeginChild('AssetTree', ui.ImVec2(ratio * ui.GetWindowSize().x, 0)) then
        for i = 1, #self.dir_tree do
            local node = self.dir_tree[i]
            self:_tree_node_action_recursive(node)
        end
    end
    ui.EndChild()
    ui.PopStyleColor()
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

asset_explorer:populate_asset_tree_list_recursive() -- Build directory tree once on start
asset_explorer:build_asset_list_in_working_dir() -- Build asset list once on start

return asset_explorer
