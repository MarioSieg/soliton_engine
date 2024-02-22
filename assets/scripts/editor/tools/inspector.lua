-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bxor = bit.bxor

local UI = require 'editor.imgui'
local ICONS = require 'editor.icons'
local Time = require 'Time'
local Scene = require 'Scene'
local Components = require 'Components'
local Vec3 = require 'Vec3'
local Quat = require 'Quat'
local EFLAGS = ENTITY_FLAGS

local MAX_TEXT_INPUT_SIZE = 128
local Inspector = {
    name = ICONS.COGS..' Inspector',
    isVisible = ffi.new('bool[1]', true),
    selectedEntity = nil,
    inputTextBuffer = ffi.new('char[?]', 1+MAX_TEXT_INPUT_SIZE),
    manipBuf3 = ffi.new('float[3]'),
    manipBufBool = ffi.new('bool[1]'),
    propertiesChanged = false,
}

local COM_NAMES = {}
for k, v in pairs(Components) do
    table.insert(COM_NAMES, k)
end
table.sort(COM_NAMES)
local COM_NAMES_C = ffi.new("const char*[?]", #COM_NAMES)
for i=1, #COM_NAMES do
    COM_NAMES_C[i-1] = ffi.cast("const char*", COM_NAMES[i])
end
local selectedComponent = ffi.new('int[1]', 0)

function Inspector:_setFloatManpitBufferFromVec3(vec3)
    self.manipBuf3[0] = vec3.x
    self.manipBuf3[1] = vec3.y
    self.manipBuf3[2] = vec3.z
end

function Inspector:_getFloatManipBufferAsVec3()
    return Vec3(self.manipBuf3[0], self.manipBuf3[1], self.manipBuf3[2])
end

function Inspector:_inspectVec3(name, vec3, step)
    step = step or 0.1
    self:_setFloatManpitBufferFromVec3(vec3)
    local changed = UI.DragFloat3(name, self.manipBuf3, step)
    if changed then
        return changed, self:_getFloatManipBufferAsVec3()
    end
    return changed, vec3
end

function Inspector:_perComponentBaseTools(instance, id)
    if UI.Button(ICONS.TRASH) then
        instance:remove()
        self.propertiesChanged = true
        return
    end
    if UI.IsItemHovered() then
        UI.SetTooltip('Remove component')
    end
    UI.SameLine()
    if UI.Button(ICONS.TRASH_RESTORE..' Reset') then
        instance:remove()
        self.selectedEntity:component(id)
        self.propertiesChanged = true
        return
    end
    if UI.IsItemHovered() then
        UI.SetTooltip('Reset component')
    end
end

function Inspector:_inspectTransform()
    local transform = self.selectedEntity:component(Components.Transform)
    if UI.CollapsingHeader(ICONS.ARROWS_ALT..' Transform', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
        self._perComponentBaseTools(transform, Components.Transform)
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
        if not entity or not entity:isValid() then
            UI.TextUnformatted('No entity selected')
        else
            UI.Combo('##ComponentType', selectedComponent, COM_NAMES_C, #COM_NAMES)
            if UI.IsItemHovered() then
                UI.SetTooltip('New component type to add')
            end
            UI.SameLine()
            if UI.Button(ICONS.PLUS..' Add') then
                entity:component(Components[COM_NAMES[selectedComponent[0]+1]])
                self.propertiesChanged = true
            end
            if UI.IsItemHovered() then
                UI.SetTooltip('Add new component')
            end
            UI.Spacing()
            UI.Separator()
            UI.Spacing()
            if UI.CollapsingHeader(ICONS.INFO_CIRCLE..' General', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                local name = entity:getName()
                if #name >= MAX_TEXT_INPUT_SIZE then
                    name = name:sub(1, MAX_TEXT_INPUT_SIZE-1)
                end
                ffi.copy(self.inputTextBuffer, name)
                if UI.InputText('Name', self.inputTextBuffer, MAX_TEXT_INPUT_SIZE) then
                    entity:setName(ffi.string(self.inputTextBuffer))
                    self.propertiesChanged = true
                end
                local hidden = entity:hasFlag(EFLAGS.HIDDEN)
                self.manipBufBool[0] = hidden
                UI.Checkbox(ICONS.EYE_SLASH..' Hidden', self.manipBufBool)
                if hidden ~= self.manipBufBool[0] then
                    entity:setFlags(bxor(entity:getFlags(), EFLAGS.HIDDEN))
                    self.propertiesChanged = true
                end
                local static = entity:hasFlag(EFLAGS.STATIC)
                self.manipBufBool[0] = static
                UI.Checkbox(ICONS.DO_NOT_ENTER..' Static', self.manipBufBool)
                if static ~= self.manipBufBool[0] then
                    entity:setFlags(bxor(entity:getFlags(), EFLAGS.STATIC))
                    self.propertiesChanged = true
                end
                local transient = entity:hasFlag(EFLAGS.TRANSIENT)
                self.manipBufBool[0] = transient
                UI.Checkbox(ICONS.ALARM_CLOCK..' Transient', self.manipBufBool)
                if transient ~= self.manipBufBool[0] then
                    entity:setFlags(bxor(entity:getFlags(), EFLAGS.TRANSIENT))
                    self.propertiesChanged = true
                end
                UI.Separator()
                UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff888888)
                UI.TextUnformatted(string.format('ID: 0x%x', tonumber(entity.id)))
                UI.TextUnformatted(string.format('Valid: %s', entity:isValid() and 'yes' or 'no'))
                UI.TextUnformatted(string.format('ID Address: %p', entity.id))
                UI.PopStyleColor()
            end
            if entity:hasComponent(Components.Transform) then
                self:_inspectTransform()
            end
        end
    end
    UI.End()
end

return Inspector
