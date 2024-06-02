-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bxor = bit.bxor

local ui = require 'editor.imgui'
local icons = require 'editor.icons'
local components = require 'components'
local gmath = require 'gmath'
local vec3 = require 'vec3'
local entity_flags = entity_flags

local max_name_text_len = 256
local inspector = {
    name = icons.i_cogs .. ' Inspector',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    properties_changed = false,
    
    _text_buf = ffi.new('char[?]', 1 + max_name_text_len),
    _vec3_buf = ffi.new('float[3]'),
    _bool_buf = ffi.new('bool[1]'),
}

-- TODO: move to gconsts
local component_names = {}
for k, _ in pairs(components) do
    table.insert(component_names, k)
end
table.sort(component_names)
local function get_component_by_name(name)
    return components[name]
end
local function get_component_by_index(i)
    return get_component_by_name(component_names[gmath.clamp(i, 1, #component_names)])
end

local component_names_with_icons = {}
for k, _ in pairs(components) do
    table.insert(component_names_with_icons, k)
end

component_names_with_icons['transform'] = icons.i_arrows_alt .. ' Transform'
component_names_with_icons['camera'] = icons.i_camera .. ' Camera'
component_names_with_icons['light'] = icons.i_lightbulb .. ' Light'

table.sort(component_names_with_icons)
local component_names_c = ffi.new("const char*[?]", #component_names_with_icons)
for i = 1, #component_names_with_icons do
    component_names_c[i - 1] = ffi.cast("const char*", component_names_with_icons[i])
end
local selected_component_idx = ffi.new('int[1]', 0)

function inspector:_set_float3_buf(vec3)
    self._vec3_buf[0] = vec3.x
    self._vec3_buf[1] = vec3.y
    self._vec3_buf[2] = vec3.z
end

function inspector:_get_float3_buf()
    return vec3(self._vec3_buf[0], self._vec3_buf[1], self._vec3_buf[2])
end

function inspector:_inspect_vec3(name, vec3, step)
    step = step or 0.1
    self:_set_float3_buf(vec3)
    local changed = ui.DragFloat3(name, self._vec3_buf, step)
    if changed then
        return changed, self:_get_float3_buf()
    end
    return changed, vec3
end

function inspector:_component_base_header(instance)
    if ui.Button(icons.i_trash) then
        if instance ~= nil then
            instance:remove()
            self.properties_changed = true
            return false
        else
            eprint('component instance is nil')
        end
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Remove component')
    end
    ui.SameLine()
    if ui.Button(icons.i_trash_restore .. ' Reset') then
        if instance ~= nil then
            instance:remove()
            -- TODO: add component again with default values
            self.properties_changed = true
            return false
        else
            eprint('component instance is nil')
        end
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Reset component')
    end
    return true
end

function inspector:_inspect_component_transform()
    local tra = self.selected_entity:get_component(components.transform)
    if ui.CollapsingHeader(icons.i_arrows_alt .. ' Transform', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
        if not self._component_base_header(tra) then
            return
        end
        local pos = tra:get_position()
        local rot = tra:get_rotation()
        local scale = tra:get_scale()
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ff88)
        local changed, pos = self:_inspect_vec3(icons.i_arrows_alt .. ' Position', pos)
        if changed then
            tra:set_position(pos)
            self.properties_changed = true
        end
        ui.PopStyleColor()
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff8888ff)
        local changed, rot = self:_inspect_vec3(icons.i_redo_alt .. ' Rotation', rot)
        if changed then
            tra:set_rotation(rot)
            self.properties_changed = true
        end
        ui.PopStyleColor()
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ffff)
        local changed, scale = self:_inspect_vec3(icons.i_expand_arrows .. ' Scale', scale)
        if changed then
            tra:set_scale(scale)
            self.properties_changed = true
        end
        ui.PopStyleColor()
    end
end

function inspector:render()
    self.properties_changed = false
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        local entity = self.selected_entity
        if not entity or not entity:is_valid() then
            ui.TextUnformatted('No entity selected')
        else
            ui.Combo('##ComponentType', selected_component_idx, component_names_c, #component_names_with_icons)
            if ui.IsItemHovered() then
                ui.SetTooltip('New component type to add')
            end
            ui.SameLine()
            if ui.Button(icons.i_plus .. ' Add') then
                entity:get_component(get_component_by_index(selected_component_idx[0] + 1))
                self.properties_changed = true
            end
            if ui.IsItemHovered() then
                ui.SetTooltip('Add new component')
            end
            if ui.CollapsingHeader(icons.i_info_circle .. ' General', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then
                local name = entity:get_name()
                if #name >= max_name_text_len then
                    name = name:sub(1, max_name_text_len-1)
                end
                ffi.copy(self._text_buf, name)
                if ui.InputText('Name', self._text_buf, max_name_text_len) then
                    entity:setName(ffi.string(self._text_buf))
                    self.properties_changed = true
                end
                local hidden = entity:has_flag(entity_flags.hidden)
                self._bool_buf[0] = hidden
                ui.Checkbox(icons.i_eye_slash .. ' Hidden', self._bool_buf)
                if hidden ~= self._bool_buf[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.hidden))
                    self.properties_changed = true
                end
                local static = entity:has_flag(entity_flags.static)
                self._bool_buf[0] = static
                ui.Checkbox(icons.i_do_not_enter .. ' Static', self._bool_buf)
                if static ~= self._bool_buf[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.static))
                    self.properties_changed = true
                end
                local transient = entity:has_flag(entity_flags.transient)
                self._bool_buf[0] = transient
                ui.Checkbox(icons.i_alarm_clock .. ' Transient', self._bool_buf)
                if transient ~= self._bool_buf[0] then
                    entity:set_flags(bxor(entity:get_flags(), entity_flags.transient))
                    self.properties_changed = true
                end
                -- ui.Separator()
                -- ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff888888)
                -- ui.TextUnformatted(string.format('ID: 0x%x', tonumber(entity.id)))
                -- ui.TextUnformatted(string.format('Valid: %s', entity:isValid() and 'yes' or 'no'))
                -- ui.TextUnformatted(string.format('ID Address: %p', entity.id))
                -- ui.PopStyleColor()
            end
            if entity:has_component(components.transform) then
                self:_inspect_component_transform()
            end
        end
    end
    ui.End()
end

return inspector
