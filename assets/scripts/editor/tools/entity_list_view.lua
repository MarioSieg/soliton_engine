-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local scene = require('scene')
local entity_flags = entity_flags

local EntityListView = {
    name = ICONS.CUBES..' Entities',
    is_visible = ffi.new('bool[1]', true),
    selectedEntity = nil,
    entityList = {},
    entityCounter = 0,
    showHiddenEntities = ffi.new('bool[1]', false),
    selectedWantsFocus = false,
}

function EntityListView:buildEntityList()
    self.entityList = {}
    scene._entity_query_start()
    for i=0, scene._entity_query_next() do
        local entity = scene._entity_query_lookup(i)
        if entity:is_valid() then
            if not self.showHiddenEntities[0] and entity:has_flag(entity_flags.hidden) then
                goto continue
            end
            local name = entity:get_name()
            local isUnnamed = name == ''
            if isUnnamed then
                name = 'Unnamed'
            end
            name = ICONS.CUBE..' '..name
            table.insert(self.entityList, {entity, name, isUnnamed})
            ::continue::
        end
    end
    scene._entity_query_end()
end

function EntityListView:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.is_visible) then
        if UI.Button(ICONS.PLUS) then
            self.entityCounter = self.entityCounter + 1
            local ent = scene.spawn('New entity '..self.entityCounter)
            self.selectedEntity = ent
            self:buildEntityList()
        end
        UI.SameLine()
        if UI.Button(ICONS.TRASH) then
            if self.selectedEntity then
                scene.despawn(self.selectedEntity)
                self.selectedEntity = nil
                self:buildEntityList()
            end
        end
        UI.SameLine()
        if UI.Button(ICONS.REDO_ALT) then
            self:buildEntityList()
        end
        UI.SameLine()
        if UI.Checkbox((self.showHiddenEntities[0] and ICONS.EYE or ICONS.EYE_SLASH), self.showHiddenEntities) then
            self:buildEntityList()
        end
        if UI.IsItemHovered() then
            UI.SetTooltip('Show hidden entities')
        end
        UI.SameLine()
        UI.Spacing()
        UI.SameLine()
        UI.Text('Entities: '..#self.entityList)
        UI.Separator()
        local size = UI.ImVec2(0, 0)
        if UI.BeginChild('EntityScrollingRegion', UI.ImVec2(0, -UI.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            UI.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, UI.ImVec2(4.0, 1.0))
            local clipper = UI.ImGuiListClipper()
            clipper:Begin(#self.entityList, UI.GetTextLineHeightWithSpacing())
            while clipper:Step() do -- HOT LOOP
                for i=clipper.DisplayStart+1, clipper.DisplayEnd do
                    local data = self.entityList[i]
                    if data[1]:is_valid() then
                        local isUnnamed = data[3]
                        local isHidden = data[1]:has_flag(entity_flags.hidden)
                        local isStatic = data[1]:has_flag(entity_flags.static)
                        local isTransient = data[1]:has_flag(entity_flags.transient)
                        local color = isHidden and 0xff888888 or isStatic and 0xffff8888 or isTransient and 0xff88ff88 or 0xffffffff
                        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, color)
                        if UI.Selectable(data[2], self.selectedEntity == data[1], 0, size) then
                            self.selectedEntity = data[1]
                        end
                        if UI.IsItemHovered() and UI.IsMouseDoubleClicked(0) then
                            self.selectedEntity = data[1]
                            self.selectedWantsFocus = true
                        end
                        UI.PopStyleColor()
                        if UI.IsItemHovered() then
                            UI.SetTooltip(isUnnamed and 'This entity has no name' or 'entity ID: 0x'..string.format('%x', tonumber(data[1].id)))
                        end
                    end
                end
            end
            clipper:End()
            UI.PopStyleVar()
            UI.EndChild()
        end
    end
    UI.End()
end

return EntityListView
