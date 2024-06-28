-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'
local bxor = bit.bxor

local ui = require 'editor.imgui'
local icons = require 'editor.icons'
local components = require 'components'
local vec3 = require 'vec3'
local quat = require 'quat'
local rad, deg = math.rad, math.deg
local entity_flags = entity_flags
local render_flags = render_flags

local max_name_text_len = 256
local header_buttons_offset = 60.0 -- TODO: calculate from button sizes
local inspector_header_flags = ffi.C.ImGuiTreeNodeFlags_DefaultOpen + ffi.C.ImGuiTreeNodeFlags_AllowOverlap
local inspector = {
    name = icons.i_cogs .. ' Inspector',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    properties_changed = false,
    name_changed = false,

    _text_buf = ffi.new('char[?]', 1 + max_name_text_len),
    _float_buf = ffi.new('float[1]'),
    _vec2_buf = ffi.new('float[2]'),
    _vec3_buf = ffi.new('float[3]'),
    _bool_buf = ffi.new('bool[1]'),
}

function inspector:_inspect_float(name, value, step, min, max, fmt)
    step = step or 0.1
    min = min or -math.huge
    max = max or math.huge
    fmt = fmt or '%.3f'
    self._float_buf[0] = value
    local updated = ui.DragFloat(name, self._float_buf, step, min, max, fmt)
    if updated then
        return updated, self._float_buf[0]
    end
    return updated, value
end

function inspector:_inspect_vec2(name, vector2, step, min, max, fmt)
    step = step or 0.1
    min = min or -math.huge
    max = max or math.huge
    fmt = fmt or '%.3f'
    self._vec2_buf[0] = vector2.x
    self._vec2_buf[1] = vector2.y
    local updated = ui.DragFloat2(name, self._vec2_buf, step, min, max, fmt)
    if updated then
        return updated, vec2(self._vec2_buf[0], self._vec2_buf[1])
    end
    return updated, vector2
end

function inspector:_inspect_vec3(name, vector3, step, min, max, fmt)
    step = step or 0.1
    min = min or -math.huge
    max = max or math.huge
    fmt = fmt or '%.3f'
    self._vec3_buf[0] = vector3.x
    self._vec3_buf[1] = vector3.y
    self._vec3_buf[2] = vector3.z
    local updated = ui.DragFloat3(name, self._vec3_buf, step, min, max, fmt)
    if updated then
        return updated, vec3(self._vec3_buf[0], self._vec3_buf[1], self._vec3_buf[2])
    end
    return updated, vector3
end

function inspector:_component_base_header()
    ui.PushStyleColor(ffi.C.ImGuiCol_Border, 0)
    ui.PushStyleColor(ffi.C.ImGuiCol_Button, 0)
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
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ff88)
        local updated, pos = self:_inspect_vec3(icons.i_arrows_alt .. ' Position', pos)
        if updated then
            c_transform:set_position(pos)
            self.properties_changed = true
        end
        ui.PopStyleColor()

        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff8888ff)
        local updated, rot = self:_inspect_vec3(icons.i_redo_alt .. ' Rotation', rot)
        if updated then
            c_transform:set_rotation(quat.from_euler(rad(rot.x), rad(rot.y), rad(rot.z)))
            self.properties_changed = true
        end
        ui.PopStyleColor()

        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff88ffff)
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
        self._bool_buf[0] = is_visible
        ui.Checkbox(icons.i_eye .. ' Visible', self._bool_buf)
        if is_visible ~= self._bool_buf[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_rendering))
            self.properties_changed = true
        end

        ui.SameLine()

        local do_frustum_culling = not c_mesh_renderer:has_flag(render_flags.skip_frustum_culling)
        self._bool_buf[0] = do_frustum_culling
        ui.Checkbox(icons.i_camera .. ' Frustum Culling', self._bool_buf)
        if do_frustum_culling ~= self._bool_buf[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_frustum_culling))
            self.properties_changed = true
        end
    end
end

function inspector:_entity_base_header(entity)
    if ui.CollapsingHeader(icons.i_cogs .. ' Entity', ffi.C.ImGuiTreeNodeFlags_DefaultOpen) then

        local name = entity:get_name()
        if #name >= max_name_text_len then
            name = name:sub(1, max_name_text_len-1)
        end
        ffi.copy(self._text_buf, name)
        if ui.InputText('Name', self._text_buf, max_name_text_len) then
            entity:set_name(ffi.string(self._text_buf))
            self.name_changed = true
        end

        local hidden = entity:has_flag(entity_flags.hidden)
        self._bool_buf[0] = hidden
        ui.Checkbox(icons.i_eye_slash .. ' Hidden', self._bool_buf)
        if hidden ~= self._bool_buf[0] then
            entity:set_flags(bxor(entity:get_flags(), entity_flags.hidden))
            self.properties_changed = true
        end

        ui.SameLine()

        local static = entity:has_flag(entity_flags.static)
        self._bool_buf[0] = static
        ui.Checkbox(icons.i_do_not_enter .. ' Static', self._bool_buf)
        if static ~= self._bool_buf[0] then
            entity:set_flags(bxor(entity:get_flags(), entity_flags.static))
            self.properties_changed = true
        end

        ui.SameLine()

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
end

function inspector:render()
    self.properties_changed = false
    self.name_changed = false
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
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
                ui.PushOverrideID(popupid_add_component)
                ui.OpenPopup(icons.i_database .. ' Component Library')
                ui.PopID()
            end
        end
    end
    ui.End()
end

return inspector
