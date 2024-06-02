-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bxor = bit.bxor

local UI = require 'editor.imgui'
local icons = require 'editor.icons'
local components = require 'components'
local vec3 = require 'vec3'
local entity_flags = entity_flags

local MAX_TEXT_INPUT_SIZE = 128
local Inspector = {
    name = icons.i_cogs .. ' Inspector',
    is_visible = ffi.new('bool[1]', true),
    selectedEntity = nil,
    inputTextBuffer = ffi.new('char[?]', 1 + MAX_TEXT_INPUT_SIZE),
    manipBuf3 = ffi.new('float[3]'),
    manipBufBool = ffi.new('bool[1]'),
    propertiesChanged = false,
}

local COM_NAMES = {}
for k, _ in pairs(components) do
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
    return vec3(self.manipBuf3[0], self.manipBuf3[1], self.manipBuf3[2])
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
    if UI.Button(icons.i_trash) then
        instance:remove()
        self.propertiesChanged = true
        return false
    end
    if UI.IsItemHovered() then
        UI.SetTooltip('Remove component')
    end
    UI.SameLine()
    if UI.Button(icons.i_trash_restore .. ' Reset') then
        instance:remove()
        self.propertiesChanged = true
        return false
    end
    if UI.IsItemHovered() then
        UI.SetTooltip('Reset component')
    end
    return true
end

function Inspector:_inspectTransform()
    local transform = self.selectedEntity:get_component(components.transform)
    if UI.CollapsingHeader(icons.i_arrows_alt .. ' transform', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
        if not self._perComponentBaseTools(transform, components.transform) then
            return
        end
        local pos = transform:get_position()
        local rot = transform:get_rotation()
        local scale = transform:get_scale()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ff88)
        local changed, pos = self:_inspectVec3(icons.i_arrows_alt .. ' Position', pos)
        if changed then
            transform:set_position(pos)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff8888ff)
        local changed, rot = self:_inspectVec3(icons.i_redo_alt .. ' Rotation', rot)
        if changed then
            transform:set_rotation(rot)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ffff)
        local changed, scale = self:_inspectVec3(icons.i_expand_arrows .. ' Scale', scale)
        if changed then
            transform:set_scale(scale)
            self.propertiesChanged = true
        end
        UI.PopStyleColor()
    end
end

function Inspector:render()
    self.propertiesChanged = false
    UI.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.is_visible) then
        local entity = self.selectedEntity
        if not entity or not entity:is_valid() then
            UI.TextUnformatted('No entity selected')
        else
            UI.Combo('##ComponentType', selectedComponent, COM_NAMES_C, #COM_NAMES)
            if UI.IsItemHovered() then
                UI.SetTooltip('New component type to add')
            end
            UI.SameLine()
            if UI.Button(icons.i_plus .. ' Add') then
                entity:get_component(components[COM_NAMES[selectedComponent[0]+1]])
                self.propertiesChanged = true
            end
            if UI.IsItemHovered() then
                UI.SetTooltip('Add new component')
            end
            if UI.CollapsingHeader(icons.i_info_circle .. ' General', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                local name = entity:get_name()
                if #name >= MAX_TEXT_INPUT_SIZE then
                    name = name:sub(1, MAX_TEXT_INPUT_SIZE-1)
                end
                ffi.copy(self.inputTextBuffer, name)
                if UI.InputText('Name', self.inputTextBuffer, MAX_TEXT_INPUT_SIZE) then
                    entity:setName(ffi.string(self.inputTextBuffer))
                    self.propertiesChanged = true
                end
                local hidden = entity:has_flag(entity_flags.hidden)
                self.manipBufBool[0] = hidden
                UI.Checkbox(icons.i_eye_slash .. ' Hidden', self.manipBufBool)
                if hidden ~= self.manipBufBool[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.hidden))
                    self.propertiesChanged = true
                end
                local static = entity:has_flag(entity_flags.static)
                self.manipBufBool[0] = static
                UI.Checkbox(icons.i_do_not_enter .. ' Static', self.manipBufBool)
                if static ~= self.manipBufBool[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.static))
                    self.propertiesChanged = true
                end
                local transient = entity:has_flag(entity_flags.transient)
                self.manipBufBool[0] = transient
                UI.Checkbox(icons.i_alarm_clock .. ' Transient', self.manipBufBool)
                if transient ~= self.manipBufBool[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.transient))
                    self.propertiesChanged = true
                end
                -- UI.Separator()
                -- UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff888888)
                -- UI.TextUnformatted(string.format('ID: 0x%x', tonumber(entity.id)))
                -- UI.TextUnformatted(string.format('Valid: %s', entity:isValid() and 'yes' or 'no'))
                -- UI.TextUnformatted(string.format('ID Address: %p', entity.id))
                -- UI.PopStyleColor()
            end
            if entity:has_component(components.transform) then
                self:_inspectTransform()
            end
        end
    end
    UI.End()
end

return Inspector
