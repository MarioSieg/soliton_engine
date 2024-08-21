-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local scene = require 'scene'
local gmath = require 'gmath'
local entity_flags = entity_flags

require 'table.clear'

local entity_list_view = {
    name = icons.i_cubes .. ' Entities',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    show_hidden_entities = ffi.new('bool[1]', false),
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
            if not self.show_hidden_entities[0] and entity:has_flag(entity_flags.hidden) then
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
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
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
        ui.SameLine()
        if ui.Checkbox((self.show_hidden_entities[0] and icons.i_eye or icons.i_eye_slash), self.show_hidden_entities) then
            self:build_entity_list()
        end
        if ui.IsItemHovered() then
            ui.SetTooltip('Show hidden entities')
        end
        ui.SameLine()
        ui.Spacing()
        ui.SameLine()
        ui.Text('Entities: ' .. #self._entity_list)
        ui.Separator()
        local size = ui.ImVec2(0, 0)
        if ui.BeginChild('EntityScrollingRegion', ui.ImVec2(0, -ui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            ui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, ui.ImVec2(4.0, 1.0))
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
                        local is_static = entity:has_flag(entity_flags.static)
                        local is_transient = entity:has_flag(entity_flags.transient)
                        local color = is_hidden and 0xff888888 or is_static and 0xffff8888 or is_transient and 0xff88ff88 or 0xffffffff
                        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, color)
                        if ui.Selectable(name, self.selected_entity == entity, 0, size) then
                            self.selected_entity = entity
                            self._selected_entity_idx = i
                        end
                        if ui.IsItemHovered() and ui.IsMouseDoubleClicked(0) then
                            self.selected_entity = entity
                            self.selected_wants_focus = true
                            self._selected_entity_idx = i
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
