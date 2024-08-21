-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local utils = require 'editor.utils'
local bor = bit.bor
require 'table.clear'

local asset_explorer = {
    name = icons.i_folder_tree .. ' Asset View',
    is_visible = ffi.new('bool[1]', true),
    root_asset_dir = 'engine_assets',
    columns_range = { min = 4, max = 15 },

    _tree_ratio = 0.2,
    _padding = 8,
    _current_working_dir = '',
    _prettified_current_working_dir = '',
    _asset_list = {},
    _dir_tree = {},
    _columns = ffi.new('int[1]', 12),
    _history = {},
    _history_idx = 1
}

local function prettify_dir(path)
    path = path:gsub('engine_assets', 'RES')
    local parts = {}
    for part in path:gmatch('[^/]+') do
        table.insert(parts, part)
    end
    return table.concat(parts, ' / ')
end

local function get_file_ext_or_fallback(path)
    local ext = string.match(path, '[^.]+$')
    if not ext then
        ext = 'unknown'
    end
    return ext
end

asset_explorer._current_working_dir = asset_explorer.root_asset_dir -- We start at the asset root
asset_explorer._prettified_current_working_dir = prettify_dir(asset_explorer._current_working_dir)
table.insert(asset_explorer._history, asset_explorer.root_asset_dir)

local dir_color = 0xffaaaaaa
local file_color = 0xffcccccc

local function is_asset_path_visible_in_editor(path)
    return path ~= '.' and path ~= '..' and not path:match("^%.") -- Ignore hidden files
end

function asset_explorer:populate_asset_tree_list_recursive(path, parent)
    path = path or self.root_asset_dir
    parent = parent or self._dir_tree
    for entry in lfs.dir(path) do
        if is_asset_path_visible_in_editor(entry) then
            local full_path = path .. '/' .. entry
            local attr = lfs.attributes(full_path)
            if attr and (attr.mode == 'directory' or attr.mode == 'file') then
                local ext = nil
                if attr.mode == 'file' then
                    ext = get_file_ext_or_fallback(entry)
                end
                local asset_type = utils.determine_asset_type(ext)
                local node = {
                    name = entry,
                    pretty_name = utils.prettify_name(entry),
                    type = asset_type,
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
        if is_asset_path_visible_in_editor(entry) then
            local full_path = path .. '/' .. entry
            local attr = lfs.attributes(full_path)
            local is_dir = attr.mode == 'directory'
            local ext = nil
            if not is_dir then
                ext = get_file_ext_or_fallback(entry)
            end
            local asset_info = {
                path = full_path,
                name = entry,
                is_dir = is_dir,
                size = attr.size,
                date = attr.modification,
                type = utils.determine_asset_type(ext)
            }
            table.insert(self._asset_list, asset_info )
        end
    end
end

function asset_explorer:build_asset_list_in_working_dir()
    table.clear(self._asset_list)
    local attribs = lfs.attributes(self._current_working_dir)
    if attribs == nil or attribs.mode ~= 'directory' then
        eprint('asset_explorer failed to scan directory: ' .. self._current_working_dir)
        return
    end
    self:build_asset_list_in_working_dir_impl(self._current_working_dir)
end

function asset_explorer:_change_asset_list_working_dir(path)
    if path == nil or type(path) ~= 'string' then
        eprint('invalid path')
        return
    end
    if path == self.root_asset_dir then -- reset history if root is reached
        table.clear(self._history)
        table.insert(self._history, self.root_asset_dir)
        self._history_idx = 1
    end
    self._current_working_dir = path
    self._prettified_current_working_dir = prettify_dir(asset_explorer._current_working_dir)
    self:build_asset_list_in_working_dir()
    table.insert(self._history, path)
    self._history_idx = #self._history
end

function asset_explorer:refresh()
    table.clear(self._dir_tree)
    self:populate_asset_tree_list_recursive() -- Build directory tree once on start
    self:build_asset_list_in_working_dir() -- Build asset list once on start
end

function asset_explorer:_tree_node_action_recursive(node)
    local flags = ffi.C.ImGuiTreeNodeFlags_SpanAvailWidth
    if node.is_file then
        flags = bor(flags, ffi.C.ImGuiTreeNodeFlags_Leaf)
    end
    local name = node.pretty_name
    local icon = utils.asset_type_icons[node.type]
    local color = utils.asset_type_colors[node.type]
    local node_open = ui.TreeNodeEx('##' .. name, flags)
    if ui.IsItemClicked() and not node.is_file then
        self:_change_asset_list_working_dir(node.path)
    end
    ui.SameLine()
    ui.PushStyleColor(ffi.C.ImGuiCol_Text, color)
    ui.TextUnformatted(icon)
    ui.PopStyleColor()
    ui.SameLine()
    ui.TextUnformatted(name)
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
        for i = 1, #self._dir_tree do
            local node = self._dir_tree[i]
            self:_tree_node_action_recursive(node)
        end
    end
    ui.EndChild()
    ui.PopStyleColor()
end

function asset_explorer:go_back_in_history()
    if self._history_idx > 1 then
        self._history_idx = self._history_idx - 1
        self:_change_asset_list_working_dir(self._history[self._history_idx])
    end
end

function asset_explorer:go_forward_in_history()
    if self._history_idx < #self._history then
        self._history_idx = self._history_idx + 1
        self:_change_asset_list_working_dir(self._history[self._history_idx])
    end
end

function asset_explorer:go_up_in_directory()
    local parts = {}
    for part in self._current_working_dir:gmatch('[^/]+') do
        table.insert(parts, part)
    end
    table.remove(parts)
    local new_path = table.concat(parts, '/')
    if new_path == '' then
        new_path = self.root_asset_dir
    end
    self:_change_asset_list_working_dir(new_path)
end

function asset_explorer:render()
    ui.SetNextWindowSize(utils.default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible, ffi.C.ImGuiWindowFlags_MenuBar) then
        if ui.BeginMenuBar() then
            if ui.SmallButton(icons.i_arrow_circle_left) then
                self:go_back_in_history()
            end
            if ui.SmallButton(icons.i_arrow_circle_right) then
                self:go_forward_in_history()
            end
            if ui.SmallButton(icons.i_arrow_circle_up) then
                self:go_up_in_directory()
            end
            ui.Separator()
            if ui.SmallButton(icons.i_redo_alt .. ' Refresh') then
                self:refresh()
            end
            ui.PushItemWidth(105)
            ui.SliderInt('##AssetExplorerColumns', self._columns, self.columns_range.min, self.columns_range.max, '%d Assets')
            ui.PopItemWidth()
            ui.Separator()
            ui.TextUnformatted(self._prettified_current_working_dir)
            ui.EndMenuBar()
        end
        local cols = self._columns[0]
        local win_size_x = (1 - self._tree_ratio) * ui.GetWindowSize().x
        local tile = (win_size_x / cols) - self._padding
        local grid_size = ui.ImVec2(tile, tile)
        self:draw_asset_tree_structure(self._tree_ratio)
        ui.SameLine()
        local rebuild_asset_list = nil
        if ui.BeginChild('AssetScrollingRegion', ui.ImVec2(win_size_x, 0), false) then
            ui.Columns(cols, 'AssetColumns', false)
            for i = 1, #self._asset_list do
                local asset = self._asset_list[i]
                local label = asset.name
                -- Render icon with file name in a grid:
                local selected = ui.Button(label, grid_size)
                if ui.IsItemHovered() and ui.IsMouseDoubleClicked(0) then -- If double click, enter directory
                    if asset.is_dir then
                        rebuild_asset_list = asset.path
                    end
                end
                if ui.IsItemHovered() then
                    ui.SetTooltip(string.format('%s - %s', label, utils.asset_type_names[asset.type]))
                end
                ui.NextColumn()
            end
            ui.EndChild()
        end
        if rebuild_asset_list ~= nil and type(rebuild_asset_list) == 'string' then
            self:_change_asset_list_working_dir(rebuild_asset_list)
        end
    end
    ui.End()
end

asset_explorer:refresh()

return asset_explorer
