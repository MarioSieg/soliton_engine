-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local components = require 'components'
local vec3 = require 'vec3'
local quat = require 'quat'
local scene = require 'scene'
local utils = require 'editor.utils'

local cpp = ffi.C
local bxor = bit.bxor
local rad, deg, inf = math.rad, math.deg, math.huge
local entity_flags = entity_flags
local render_flags = render_flags
local color_pick_flags = cpp.ImGuiColorEditFlags_NoAlpha + cpp.ImGuiColorEditFlags_Float + cpp.ImGuiColorEditFlags_InputRGB + cpp.ImGuiColorEditFlags_PickerHueWheel
local max_name_text_len = 256
local header_buttons_offset = 60.0 -- TODO: calculate from button sizes
local inspector_header_flags = cpp.ImGuiTreeNodeFlags_DefaultOpen + cpp.ImGuiTreeNodeFlags_AllowOverlap
local inspector = {
    name = icons.i_cogs .. ' Inspector',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    env_editor = false,
    properties_changed = false,
    name_changed = false,

    _tmp_text = ffi.new('char[?]', 1 + max_name_text_len),
    _tmp_float = ffi.new('float[1]'),
    _tmp_vec2 = ffi.new('float[2]'),
    _tmp_vec3 = ffi.new('float[3]'),
    _tmp_boolean = ffi.new('bool[1]'),
}

function inspector:_inspect_float(name, x, step, min, max, fmt, slider)
    self._tmp_float[0] = x
    local updated = false
    if slider then
        updated = ui.SliderFloat(name, self._tmp_float, min or -inf, max or inf, fmt or '%.3f')
    else
        updated = ui.DragFloat(name, self._tmp_float, step or 0.1, min or -inf, max or inf, fmt or '%.3f')
    end
    if updated then
        return true, self._tmp_float[0]
    end
    return true, x
end

function inspector:_inspect_vec2(name, v2, step, min, max, fmt)
    self._tmp_vec2[0] = v2.x
    self._tmp_vec2[1] = v2.y
    local updated = ui.DragFloat2(name, self._tmp_vec2, step or 0.1, min or -inf, max or inf, fmt or '%.3f')
    if updated then
        return true, vec2(self._tmp_vec2[0], self._tmp_vec2[1])
    end
    return false, v2
end

function inspector:_inspect_vec3(name, v3, step, min, max, fmt)
    self._tmp_vec3[0] = v3.x
    self._tmp_vec3[1] = v3.y
    self._tmp_vec3[2] = v3.z
    local updated = ui.DragFloat3(name, self._tmp_vec3, step or 0.1, min or -inf, max or inf, fmt or '%.3f')
    if updated then
        return true, vec3(self._tmp_vec3[0], self._tmp_vec3[1], self._tmp_vec3[2])
    end
    return false, v3
end

function inspector:_inspect_vec3_color_rgb(name, v3)
    self._tmp_vec3[0] = v3.x
    self._tmp_vec3[1] = v3.y
    self._tmp_vec3[2] = v3.z
    local updated = ui.ColorEdit3(name, self._tmp_vec3, color_pick_flags)
    if updated then
        return true, vec3(self._tmp_vec3[0], self._tmp_vec3[1], self._tmp_vec3[2])
    end
    return false, v3
end

function inspector:_component_base_header()
    ui.PushStyleColor(cpp.ImGuiCol_Border, 0)
    ui.PushStyleColor(cpp.ImGuiCol_Button, 0)
    ui.SameLine(ui.GetWindowWidth() - header_buttons_offset)
    local keep_component = true
    if ui.SmallButton(icons.i_trash_restore) then
        self.properties_changed = true
        keep_component = false
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Reset component to default values')
    end
    ui.SameLine()
    if ui.SmallButton(icons.i_trash) then
        self.properties_changed = true
        keep_component = false
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Remove component')
    end
    ui.PopStyleColor(2)
    return keep_component
end

function inspector:_inspect_component_transform()
    local c_transform = self.selected_entity:get_component(components.transform)
    if ui.CollapsingHeader(icons.i_arrows_alt .. ' Transform', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(components.transform)
            return
        end

        local pos = c_transform:get_position()
        local rx, ry, rz = quat.to_euler(c_transform:get_rotation())
        local rot = vec3(deg(rx), deg(ry), deg(rz))
        local scale = c_transform:get_scale()
        ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff88ff88)
        local updated, pos = self:_inspect_vec3(icons.i_arrows_alt .. ' Position', pos)
        if updated then
            c_transform:set_position(pos)
            self.properties_changed = true
        end
        ui.PopStyleColor()

        ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff8888ff)
        local updated, rot = self:_inspect_vec3(icons.i_redo_alt .. ' Rotation', rot)
        if updated then
            c_transform:set_rotation(quat.from_euler(rad(rot.x), rad(rot.y), rad(rot.z)))
            self.properties_changed = true
        end
        ui.PopStyleColor()

        ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff88ffff)
        local updated, scale = self:_inspect_vec3(icons.i_expand_arrows .. ' Scale', scale)
        if updated then
            c_transform:set_scale(scale)
            self.properties_changed = true
        end
        ui.PopStyleColor()
    end
end

function inspector:_inspect_component_camera()
    local c_camera = self.selected_entity:get_component(components.camera)
    if ui.CollapsingHeader(icons.i_camera .. ' Camera', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(components.camera)
            return
        end

        local fov = c_camera:get_fov()
        local updated, fov = self:_inspect_float(icons.i_eye .. ' FOV', fov, 0.1, 1.0, 180.0, '%.0f')
        if updated then
            c_camera:set_fov(fov)
            self.properties_changed = true
        end

        local near_z_clip = c_camera:get_near_clip()
        local updated, near_z_clip = self:_inspect_float(icons.i_sign_in_alt .. ' Near Clip', near_z_clip, 1.0, 0.1, 10000.0, '%.0f')
        if updated then
            c_camera:set_near_clip(near_z_clip)
            self.properties_changed = true
        end

        local far_z_clip = c_camera:get_far_clip()
        local updated, far_z_clip = self:_inspect_float(icons.i_sign_out_alt .. ' Far Clip', far_z_clip, 1.0, 0.1, 10000.0, '%.0f')
        if updated then
            c_camera:set_far_clip(far_z_clip)
            self.properties_changed = true
        end
    end
end

function inspector:_inspect_component_mesh_renderer()
    local c_mesh_renderer = self.selected_entity:get_component(components.mesh_renderer)
    if ui.CollapsingHeader(icons.i_cube .. ' Mesh Renderer', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(components.mesh_renderer)
            return
        end

        local is_visible = not c_mesh_renderer:has_flag(render_flags.skip_rendering)
        self._tmp_boolean[0] = is_visible
        ui.Checkbox(icons.i_eye .. ' Visible', self._tmp_boolean)
        if is_visible ~= self._tmp_boolean[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_rendering))
            self.properties_changed = true
        end

        ui.SameLine()

        local do_frustum_culling = not c_mesh_renderer:has_flag(render_flags.skip_frustum_culling)
        self._tmp_boolean[0] = do_frustum_culling
        ui.Checkbox(icons.i_camera .. ' Frustum Culling', self._tmp_boolean)
        if do_frustum_culling ~= self._tmp_boolean[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_frustum_culling))
            self.properties_changed = true
        end
    end
end

function inspector:_entity_base_header(entity)
    if ui.CollapsingHeader(icons.i_cogs .. ' Entity', cpp.ImGuiTreeNodeFlags_DefaultOpen) then

        local name = entity:get_name()
        if #name >= max_name_text_len then
            name = name:sub(1, max_name_text_len-1)
        end
        ffi.copy(self._tmp_text, name)
        if ui.InputText('Name', self._tmp_text, max_name_text_len) then
            entity:set_name(ffi.string(self._tmp_text))
            self.name_changed = true
        end

        local hidden = entity:has_flag(entity_flags.hidden)
        self._tmp_boolean[0] = hidden
        ui.Checkbox(icons.i_eye_slash .. ' Hidden', self._tmp_boolean)
        if hidden ~= self._tmp_boolean[0] then
            entity:set_flags(bxor(entity:get_flags(), entity_flags.hidden))
            self.properties_changed = true
        end

        ui.SameLine()

        local static = entity:has_flag(entity_flags.static)
        self._tmp_boolean[0] = static
        ui.Checkbox(icons.i_do_not_enter .. ' Static', self._tmp_boolean)
        if static ~= self._tmp_boolean[0] then
            entity:set_flags(bxor(entity:get_flags(), entity_flags.static))
            self.properties_changed = true
        end

        ui.SameLine()

        local transient = entity:has_flag(entity_flags.transient)
        self._tmp_boolean[0] = transient
        ui.Checkbox(icons.i_alarm_clock .. ' Transient', self._tmp_boolean)
        if transient ~= self._tmp_boolean[0] then
            entity:set_flags(bxor(entity:get_flags(), entity_flags.transient))
            self.properties_changed = true
        end

        -- ui.Separator()
        -- ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff888888)
        -- ui.TextUnformatted(string.format('ID: 0x%x', tonumber(entity.id)))
        -- ui.TextUnformatted(string.format('Valid: %s', entity:isValid() and 'yes' or 'no'))
        -- ui.TextUnformatted(string.format('ID Address: %p', entity.id))
        -- ui.PopStyleColor()
    end
end

function inspector:_inspect_env_editor()
    if ui.CollapsingHeader(icons.i_moon_stars .. ' Timecycle', cpp.ImGuiTreeNodeFlags_DefaultOpen) then
        local _, value = self:_inspect_float(icons.i_clock .. ' Time of Day', scene.clock.date.time, nil, 0.0, 24.0, '%.2f h', true)
        scene.clock.date.time = value

        local _, value = self:_inspect_float(icons.i_alarm_clock .. ' Time Scale', scene.clock.time_cycle_scale, nil, 0.0, 1.0, nil, true)
        scene.clock.time_cycle_scale = value
    end
    if ui.CollapsingHeader(icons.i_lamp .. ' Lighting', cpp.ImGuiTreeNodeFlags_DefaultOpen) then
        local _, value = self:_inspect_vec3_color_rgb(icons.i_sun .. ' Sun Color', scene.lighting.sun_color)
        scene.lighting.sun_color = value

        local _, value = self:_inspect_vec3_color_rgb(icons.i_lightbulb .. ' Ambient Color', scene.lighting.ambient_color)
        scene.lighting.ambient_color = value

        local _, value = self:_inspect_float(icons.i_clouds .. ' Atmospheric Turbidity', scene.lighting.sky_turbidity, nil, 1.8, 10.0, nil, true)
        scene.lighting.sky_turbidity = value
    end
end

function inspector:_inspect_entity()
    local entity = self.selected_entity
    if not entity or not entity:is_valid() then
        ui.TextUnformatted('No entity selected')
    else
        self:_entity_base_header(entity)
        if entity:has_component(components.transform) then
            self:_inspect_component_transform()
        end
        if entity:has_component(components.camera) then
            self:_inspect_component_camera()
        end
        if entity:has_component(components.mesh_renderer) then
            self:_inspect_component_mesh_renderer()
        end
        ui.Spacing()
        ui.Separator()
        ui.Spacing()
        if ui.Button(icons.i_plus_circle .. ' Add Component') then
            ui.PushOverrideID(utils.popupid_add_component)
            ui.OpenPopup(icons.i_database .. ' Component Library')
            ui.PopID()
        end
    end
end

function inspector:render()
    self.properties_changed = false
    self.name_changed = false
    ui.SetNextWindowSize(utils.default_window_size, cpp.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        if self.env_editor then
            self:_inspect_env_editor()
        else
            self:_inspect_entity()
        end
    end
    ui.End()
end

return inspector
