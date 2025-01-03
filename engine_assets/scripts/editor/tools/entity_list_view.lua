-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local scene = require 'scene'
local gmath = require 'gmath'
local utils = require 'editor.utils'
local entity_flags = entity_flags

require 'table.clear'

local entity_list_view = {
    name = icons.i_cubes .. ' Entities',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    selected_mode = 'entity', -- entity, env_editor, etc.
    selected_wants_focus = false,
    
    _entity_list = {},
    _selected_entity_idx = 1,
    _entity_acc = 0,
}

function entity_list_view:update_name_of_active_entity()
    if self.selected_entity ~= nil then
        local name = self.selected_entity:get_name()
        if name == '' then
            name = 'Unnamed'
        end
        self._entity_list[self._selected_entity_idx].name = icons.i_cube .. ' ' .. name
    end
end

function entity_list_view:build_entity_list()
    table.clear(self._entity_list)
    self.selected_entity = nil
    scene._entity_query_start()
    for i = 0, scene._entity_query_next() - 1 do
        local entity = scene._entity_query_lookup(i)
        if entity:is_valid() then
            if entity:has_flag(entity_flags.hidden) then
                goto continue
            end
            local name = entity:get_name()
            local is_anonymous = name == ''
            if is_anonymous then
                name = 'Unnamed'
            end
            name = string.format('%s %s', icons.i_cube, name)
            local entity_info = {
                entity = entity,
                name = name,
                is_anonymous = is_anonymous
            }
            table.insert(self._entity_list, entity_info)
            ::continue::
        end
    end
    scene._entity_query_end()
    -- sort alphabetically
    table.sort(self._entity_list, function(a, b) return a.name:lower() < b.name:lower() end)
    if gmath.within_interval(self._selected_entity_idx, 1, #self._entity_list) then
        self.selected_entity = self._entity_list[self._selected_entity_idx].entity
    elseif #self._entity_list > 0 then
        self._selected_entity_idx = #self._entity_list
        self.selected_entity = self._entity_list[self._selected_entity_idx].entity
    end
end

function entity_list_view:render()
    ui.SetNextWindowSize(utils.default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        if ui.Button(icons.i_plus) then
            self._entity_acc = self._entity_acc + 1
            local new_ent = scene.spawn('New entity ' .. self._entity_acc)
            self:build_entity_list()
            for i = 1, #self._entity_list do
                if self._entity_list[i].entity.id == new_ent.id then
                    self._selected_entity_idx = i
                    self.selected_entity = self._entity_list[i].entity
                    break
                end
            end
        end
        ui.SameLine()
        if ui.Button(icons.i_trash) then
            if self.selected_entity ~= nil then
                self.selected_entity:despawn()
                self.selected_entity = nil
                self:build_entity_list()
                if #self._entity_list ~= 0 then
                    if #self._entity_list >= 1 and self._selected_entity_idx > 1 then
                        self._selected_entity_idx = self._selected_entity_idx - 1
                        self.selected_entity = self._entity_list[self._selected_entity_idx].entity
                    else
                        self._selected_entity_idx = #self._entity_list
                        self.selected_entity = self._entity_list[self._selected_entity_idx].entity
                    end
                end
            end
        end
        ui.SameLine()
        if ui.Button(icons.i_redo_alt) then
            self:build_entity_list()
        end
        ui.Separator()
        local size = ui.ImVec2(0, 0)
        if ui.BeginChild('EntityScrollingRegion', ui.ImVec2(0, -ui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            ui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, ui.ImVec2(4.0, 1.0))

            -- Builtin pseudo-entities
            ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xffe6d8ad)
            if ui.Selectable(icons.i_clouds_moon .. ' Environment', self.selected_mode == 'env_editor', 0, size) then
                self.selected_mode = 'env_editor'
            end
            ui.PopStyleColor()

            ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xffbbeebb)
            if ui.Selectable(icons.i_cogs .. ' Scene Settings', self.selected_mode == 'scene_cfg_editor', 0, size) then
                self.selected_mode = 'scene_cfg_editor'
            end
            ui.PopStyleColor()

            ui.Spacing()
            ui.Separator()
            ui.Spacing()

            -- Scene entities
            local clipper = ui.ImGuiListClipper()
            clipper:Begin(#self._entity_list, ui.GetTextLineHeightWithSpacing())
            while clipper:Step() do
                for i = clipper.DisplayStart + 1, clipper.DisplayEnd do
                    local tuple = self._entity_list[i]
                    local entity = tuple.entity
                    if entity:is_valid() then
                        local name = tuple.name
                        local is_anonymous = tuple.is_anonymous
                        local is_hidden = entity:has_flag(entity_flags.hidden)
                        local color = is_hidden or 0xffffffff
                        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, color)
                        if ui.Selectable(name, self.selected_entity == entity and self.selected_mode == 'entity', 0, size) then
                            self.selected_entity = entity
                            self._selected_entity_idx = i
                            self.selected_mode = 'entity'
                        end
                        if ui.IsItemHovered() and ui.IsMouseDoubleClicked(0) then
                            self.selected_entity = entity
                            self.selected_wants_focus = true
                            self._selected_entity_idx = i
                            self.selected_mode = 'entity'
                        end
                        ui.PopStyleColor()
                        if ui.IsItemHovered() then
                            ui.SetTooltip(is_anonymous and 'This entity has no name' or string.format('ID: %x', tonumber(entity._id)))
                        end
                    end
                end
            end
            clipper:End()
            ui.PopStyleVar()
            ui.EndChild()
        end
    end
    ui.End()
end

return entity_list_view
