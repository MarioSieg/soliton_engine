-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require('Time')
local Scene = require('Scene')
local EFLAGS = ENTITY_FLAGS

local EntityListView = {
    name = ICONS.CUBES..' Entities',
    isVisible = ffi.new('bool[1]', true),
    selectedEntity = nil,
    entityList = {},
    entityCounter = 0,
    showHiddenEntities = ffi.new('bool[1]', false)
}

function EntityListView:buildEntityList()
    self.entityList = {}
    Scene.fullEntityQueryStart()
    while true do
        local n = Scene.fullEntityQueryNextTable()
        if n == 0 then break end
        for i=0, n do
            local entity = Scene.fullEntityQueryGet(i)
            if entity:isValid() then
                if not self.showHiddenEntities[0] and entity:hasFlag(EFLAGS.HIDDEN) then
                    goto continue
                end
                local name = entity:getName()
                local isUnnamed = name == ''
                if isUnnamed then
                    name = 'Unnamed'
                end
                name = ICONS.DATABASE..' '..name
                table.insert(self.entityList, {entity, name, isUnnamed})
                ::continue::
            end
        end
    end
    Scene.fullEntityQueryEnd()
end

function EntityListView:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.isVisible) then
        if UI.Button(ICONS.PLUS..' Create') then
            self.entityCounter = self.entityCounter + 1
            local ent = Scene.spawn('New Entity '..self.entityCounter)
            --self.selectedEntity = ent
            self:buildEntityList()
        end
        UI.SameLine()
        if UI.Button(ICONS.REDO_ALT..' Refresh') then
            self:buildEntityList()
        end
        UI.SameLine()
        if UI.Checkbox(ICONS.EYE..' Hidden', self.showHiddenEntities) then
            self:buildEntityList()
        end
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
                    if data[1]:isValid() then
                        local isUnnamed = data[3]
                        local isHidden = data[1]:hasFlag(EFLAGS.HIDDEN)
                        local isStatic = data[1]:hasFlag(EFLAGS.STATIC)
                        local isTransient = data[1]:hasFlag(EFLAGS.TRANSIENT)
                        local color = isHidden and 0xff888888 or isStatic and 0xffff8888 or isTransient and 0xff88ff88 or 0xffffffff
                        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, color)
                        if UI.Selectable(data[2], self.selectedEntity == data[1], 0, size) then
                            --self.selectedEntity = data[1]
                        end
                        UI.PopStyleColor()
                        if UI.IsItemHovered() then
                            UI.SetTooltip(isUnnamed and 'This entity has no name' or 'Entity ID: 0x'..string.format('%x', tonumber(data[1].id)))
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
