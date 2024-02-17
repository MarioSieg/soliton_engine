-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local profile = require 'jit.p'

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require 'Time'
local Scene = require 'Scene'
local Components = require 'Components'
local Vec3 = require 'Vec3'
local Quat = require 'Quat'

local MAX_TEXT_INPUT_SIZE = 512
local Inspector = {
    name = ICONS.COGS..' Inspector',
    isVisible = ffi.new('bool[1]', true),
    selectedEntity = nil,
    inputTextBuffer = ffi.new('char[?]', 1+MAX_TEXT_INPUT_SIZE),
    manpitBuf3 = ffi.new('float[3]'),
    propertiesChanged = false
}

function Inspector:_setFloatManpitBufferFromVec3(vec3)
    self.manpitBuf3[0] = vec3.x
    self.manpitBuf3[1] = vec3.y
    self.manpitBuf3[2] = vec3.z
end

function Inspector:_getFloatManipBufferAsVec3()
    return Vec3(self.manpitBuf3[0], self.manpitBuf3[1], self.manpitBuf3[2])
end

function Inspector:_inspectVec3(name, vec3, step)
    step = step or 0.1
    self:_setFloatManpitBufferFromVec3(vec3)
    local changed = UI.DragFloat3(name, self.manpitBuf3, step)
    if changed then
        return changed, self:_getFloatManipBufferAsVec3()
    end
    return changed, vec3
end

function Inspector:_inspectTransform()
    local transform = self.selectedEntity:component(Components.Transform)
    if UI.CollapsingHeader(ICONS.ARROWS_ALT..' Transform', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
        local pos = transform:getPosition()
        local rot = transform:getRotation()
        local scale = transform:getScale()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ff88)
        local changed, pos = self:_inspectVec3(ICONS.ARROWS_ALT..' Position', pos)
        if changed then
            transform:setPosition(pos)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff8888ff)
        local changed, rot = self:_inspectVec3(ICONS.REDO_ALT..' Rotation', rot)
        if changed then
            transform:setRotation(rot)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ffff)
        local changed, scale = self:_inspectVec3(ICONS.EXPAND_ARROWS..' Scale', scale)
        if changed then
            transform:setScale(scale)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
    end
end

function Inspector:render()
    self.propertiesChanged = false
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.isVisible) then
        local entity = self.selectedEntity
        if not entity then
            UI.TextUnformatted('No entity selected')
        else
            if UI.CollapsingHeader(ICONS.INFO_CIRCLE..' General', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                local name = entity:getName()
                local len = math.min(#name, MAX_TEXT_INPUT_SIZE)
                ffi.copy(self.inputTextBuffer, name, len)
                if UI.InputText('Name', self.inputTextBuffer, MAX_TEXT_INPUT_SIZE) then
                    entity:setName(ffi.string(self.inputTextBuffer))
                    self.propertiesChanged = true
                end
                UI.Separator()
                UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff888888)
                UI.TextUnformatted(string.format('ID: 0x%x', tonumber(entity.id)))
                UI.TextUnformatted(string.format('Valid: %s', entity:isValid() and 'yes' or 'no'))
                UI.TextUnformatted(string.format('ID Address: %p', entity.id))
                UI.PopStyleColor()
            end
            if entity:isValid() then
                if entity:hasComponent(Components.Transform) then
                    self:_inspectTransform()
                end
            end
        end
    end
    UI.End()
end

return Inspector
