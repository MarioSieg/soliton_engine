-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ui = require 'editor.imgui'
local icons = require 'editor.icons'
local scene = require('scene')
local entity_flags = entity_flags

local entity_list_view = {
    name = icons.i_cubes .. ' Entities',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    show_hidden_entities = ffi.new('bool[1]', false),
    selected_wants_focus = false,
    
    _entity_list = {},
    _entity_acc = 0,
}

function entity_list_view:build_entity_list()
    self._entity_list = {}
    scene._entity_query_start()
    for i = 0, scene._entity_query_next() do
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
            name = icons.i_cube .. ' ' .. name
            table.insert(self._entity_list, { entity=entity, name=name, is_anonymous=is_anonymous })
            ::continue::
        end
    end
    scene._entity_query_end()
end

function entity_list_view:render()
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        if ui.Button(icons.i_plus) then
            self._entity_acc = self._entity_acc + 1
            scene.spawn('New entity ' .. self._entity_acc)
            self:build_entity_list()
            self.selected_entity = self._entity_list[#self._entity_list].entity
        end
        ui.SameLine()
        if ui.Button(icons.i_trash) then
            if self.selected_entity ~= nil then
                self.selected_entity:despawn()
                self.selected_entity = nil
                self:build_entity_list()
                if #self._entity_list ~= 0 then
                    self.selected_entity = self._entity_list[#self._entity_list].entity
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
                    local ent = tuple.entity
                    if ent:is_valid() then
                        local name = tuple.name
                        local is_anonymous = tuple.is_anonymous
                        local is_hidden = ent:has_flag(entity_flags.hidden)
                        local is_static = ent:has_flag(entity_flags.static)
                        local is_transient = ent:has_flag(entity_flags.transient)
                        local color = is_hidden and 0xff888888 or is_static and 0xffff8888 or is_transient and 0xff88ff88 or 0xffffffff
                        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, color)
                        if ui.Selectable(name, self.selected_entity == ent, 0, size) then
                            self.selected_entity = ent
                        end
                        if ui.IsItemHovered() and ui.IsMouseDoubleClicked(0) then
                            self.selected_entity = ent
                            self.selected_wants_focus = true
                        end
                        ui.PopStyleColor()
                        if ui.IsItemHovered() then
                            ui.SetTooltip(is_anonymous and 'This entity has no name' or string.format('entity ID: 0x %x', ent.id))
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
