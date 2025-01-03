-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local bit = require 'bit'

local ui = require 'imgui.imgui'
local icons = require 'imgui.icons'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local quat = require 'quat'
local scene = require 'scene'
local gmath = require 'gmath'
local utils = require 'editor.utils'
local c_transform = require 'components.transform'
local c_camera = require 'components.camera'
local c_mesh_renderer = require 'components.mesh_renderer'

local cpp = ffi.C
local band, bxor = bit.band, bit.bxor
local rad, deg, inf = math.rad, math.deg, math.huge
local clamp = gmath.clamp
local entity_flags = entity_flags
local render_flags = render_flags
local color_pick_flags = cpp.ImGuiColorEditFlags_NoAlpha + cpp.ImGuiColorEditFlags_Float + cpp.ImGuiColorEditFlags_InputRGB + cpp.ImGuiColorEditFlags_PickerHueWheel
local max_name_text_len = 256
local header_buttons_offset = 60.0 -- TODO: calculate from button sizes
local inspector_header_flags = cpp.ImGuiTreeNodeFlags_DefaultOpen + cpp.ImGuiTreeNodeFlags_AllowOverlap

-- The inspector can be used to edit the env_editor settings, scene config or the selected entity.
local inspector = {
    name = icons.i_cogs .. ' Inspector',
    is_visible = ffi.new('bool[1]', true),
    selected_entity = nil,
    inspector_mode = 'entity', -- 'entity' or 'env_editor' etc..
    name_changed = false,

    _tmp_text = ffi.new('char[?]', 1 + max_name_text_len), -- +1 for null terminator
    _tmp_f64 = ffi.new('double[1]'),
    _tmp_vec2 = ffi.new('double[2]'),
    _tmp_vec3 = ffi.new('double[3]'),
    _tmp_rgb = ffi.new('float[3]'),
    _tmp_boolean = ffi.new('bool[1]'),
    _tmp_min = ffi.new('double[1]'),
    _tmp_max = ffi.new('double[1]'),
}

-- The inspect functions are used to display and edit the values of the selected entity and its components.
-- The functions return the new value if it was changed, otherwise the old value is returned.

function inspector:_inspect_string(name, str)
    if #str >= max_name_text_len then
        str = str:sub(1, max_name_text_len) -- Truncate string if too long for buffer
    end
    ffi.copy(self._tmp_text, str)
    if ui.InputText(name, self._tmp_text, max_name_text_len) then
        return ffi.string(self._tmp_text) -- Return new string if changed
    end
    return str
end

function inspector:_inspect_bit_flags(name, flags, flag)
    local is_set = band(flags, flag) ~= 0 -- Check if flag is set
    self._tmp_boolean[0] = is_set
    ui.Checkbox(name, self._tmp_boolean)
    if is_set ~= self._tmp_boolean[0] then
        flags = bxor(flags, flag) -- Toggle flag if changed
    end
    return flags
end

local function sanitize_f64(x)
    if x ~= x then return 0.0 end -- NaN
    if x == inf or x == -inf then return 0.0 end -- Inf
    return x
end

function inspector:_inspect_float(name, x, step, min, max, fmt, slider)
    self._tmp_f64[0] = sanitize_f64(x)
    self._tmp_min[0] = min or -inf
    self._tmp_max[0] = max or inf
    if slider then -- Use slider or drag input
        ui.SliderScalar(name, ffi.C.ImGuiDataType_Double, self._tmp_f64, self._tmp_min, self._tmp_max, fmt or '%.3f')
    else
        ui.DragScalar(name,  ffi.C.ImGuiDataType_Double, self._tmp_f64, step or 0.1, self._tmp_min, self._tmp_max,fmt or '%.3f')
    end
    return sanitize_f64(self._tmp_f64[0])
end

function inspector:_inspect_vec2(name, v2, step, min, max, fmt)
    self._tmp_vec2[0] = sanitize_f64(v2.x)
    self._tmp_vec2[1] = sanitize_f64(v2.y)
    self._tmp_min[0] = min or -inf
    self._tmp_max[0] = max or inf
    ui.DragScalarN(name,  ffi.C.ImGuiDataType_Double, self._tmp_vec2, 2, step or 0.1, self._tmp_min, self._tmp_max, fmt or '%.3f')
    return vec2(
        sanitize_f64(self._tmp_vec2[0]),
        sanitize_f64(self._tmp_vec2[1])
    )
end

function inspector:_inspect_vec3(name, v3, step, min, max, fmt)
    self._tmp_vec3[0] = sanitize_f64(v3.x)
    self._tmp_vec3[1] = sanitize_f64(v3.y)
    self._tmp_vec3[2] = sanitize_f64(v3.z)
    self._tmp_min[0] = min or -inf
    self._tmp_max[0] = max or inf
    ui.DragScalarN(name, ffi.C.ImGuiDataType_Double, self._tmp_vec3, 3, step or 0.1, self._tmp_min, self._tmp_max, fmt or '%.3f')
    return vec3(
        sanitize_f64(self._tmp_vec3[0]),
        sanitize_f64(self._tmp_vec3[1]),
        sanitize_f64(self._tmp_vec3[2])
    )
end

function inspector:_inspect_vec3_color_rgb(name, v3)
    self._tmp_rgb[0] = sanitize_f64(v3.x)
    self._tmp_rgb[1] = sanitize_f64(v3.y)
    self._tmp_rgb[2] = sanitize_f64(v3.z)
    ui.ColorEdit3(name, self._tmp_rgb, color_pick_flags)
    return vec3(
        sanitize_f64(self._tmp_rgb[0]),
        sanitize_f64(self._tmp_rgb[1]),
        sanitize_f64(self._tmp_rgb[2])
    )
end

function inspector:_component_base_header()
    ui.PushStyleColor(cpp.ImGuiCol_Border, 0)
    ui.PushStyleColor(cpp.ImGuiCol_Button, 0)
    ui.SameLine(ui.GetWindowWidth() - header_buttons_offset)
    local keep_component = true
    if ui.SmallButton(icons.i_trash_restore) then
        keep_component = false
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Reset component to default values')
    end
    ui.SameLine()
    if ui.SmallButton(icons.i_trash) then
        keep_component = false
    end
    if ui.IsItemHovered() then
        ui.SetTooltip('Remove component')
    end
    ui.PopStyleColor(2)
    return keep_component
end

function inspector:_inspect_component_transform()
    local c_transform = self.selected_entity:get_component(c_transform)
    if ui.CollapsingHeader(icons.i_arrows_alt .. ' Transform', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(c_transform)
            return
        end

        ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff88ff88)
        c_transform:set_position(self:_inspect_vec3(icons.i_arrows_alt .. ' Position', c_transform:get_position()))
        ui.PopStyleColor()

        -- TODO: Implement correct rotation quaternion inspector
        --ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff8888ff)
        --local rx, ry, rz = quat.to_euler(c_transform:get_rotation())
        --local rot = self:_inspect_vec3(icons.i_redo_alt .. ' Rotation', vec3(deg(rx), deg(ry), deg(rz)))
        --rot.x = rad(clamp(rot.x, -180, 180))
        --rot.y = rad(clamp(rot.y, -180, 180))
        --rot.z = rad(clamp(rot.z, -180, 180))
        --c_transform:set_rotation(quat.normalize(quat.from_euler(rot.x, rot.y, rot.z)))
        --ui.PopStyleColor()

        ui.PushStyleColor_U32(cpp.ImGuiCol_Text, 0xff88ffff)
        c_transform:set_scale(self:_inspect_vec3(icons.i_expand_arrows .. ' Scale', c_transform:get_scale()))
        ui.PopStyleColor()
    end
end

function inspector:_inspect_component_camera()
    local c_camera = self.selected_entity:get_component(c_camera)
    if ui.CollapsingHeader(icons.i_camera .. ' Camera', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(c_camera)
            return
        end

        c_camera:set_fov(self:_inspect_float(icons.i_eye .. ' FOV', c_camera:get_fov(), 0.1, 1.0, 180.0, '%.0f'))
        c_camera:set_near_clip(self:_inspect_float(icons.i_sign_in_alt .. ' Near Clip', c_camera:get_near_clip(), 1.0, 0.1, 10000.0, '%.0f'))
        c_camera:set_far_clip(self:_inspect_float(icons.i_sign_out_alt .. ' Far Clip', c_camera:get_far_clip(), 1.0, 0.1, 10000.0, '%.0f'))
    end
end

function inspector:_inspect_component_mesh_renderer()
    local c_mesh_renderer = self.selected_entity:get_component(c_mesh_renderer)
    if ui.CollapsingHeader(icons.i_cube .. ' Mesh Renderer', inspector_header_flags) then
        if not self:_component_base_header() then
            self.selected_entity:remove_component(c_mesh_renderer)
            return
        end

        local is_visible = not c_mesh_renderer:has_flag(render_flags.skip_rendering)
        self._tmp_boolean[0] = is_visible
        ui.Checkbox(icons.i_eye .. ' Visible', self._tmp_boolean)
        if is_visible ~= self._tmp_boolean[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_rendering))
        end

        ui.SameLine()

        local do_frustum_culling = not c_mesh_renderer:has_flag(render_flags.skip_frustum_culling)
        self._tmp_boolean[0] = do_frustum_culling
        ui.Checkbox(icons.i_camera .. ' Frustum Culling', self._tmp_boolean)
        if do_frustum_culling ~= self._tmp_boolean[0] then
            c_mesh_renderer:set_flags(bxor(c_mesh_renderer:get_flags(), render_flags.skip_frustum_culling))
        end
    end
end

function inspector:_entity_base_header(entity)
    if ui.CollapsingHeader(icons.i_cogs .. ' Entity', cpp.ImGuiTreeNodeFlags_DefaultOpen) then

        local name = entity:get_name()
        local new_name = self:_inspect_string(icons.i_tag .. ' Name', name)
        if new_name ~= name then
            entity:set_name(new_name)
            self.name_changed = true
        end

        entity:set_flags(self:_inspect_bit_flags(icons.i_eye_slash .. ' Hidden', entity:get_flags(), entity_flags.hidden))
        ui.SameLine()
        entity:set_flags(self:_inspect_bit_flags(icons.i_do_not_enter .. ' Static', entity:get_flags(), entity_flags.static))
        ui.SameLine()
        entity:set_flags(self:_inspect_bit_flags(icons.i_alarm_clock .. ' Transient', entity:get_flags(), entity_flags.transient))
    end
end

-- Inspect the env_editor settings.
function inspector:_inspect_env_editor()
    if ui.CollapsingHeader(icons.i_moon_stars .. ' Timecycle', cpp.ImGuiTreeNodeFlags_DefaultOpen) then
        scene.chrono.time = self:_inspect_float(icons.i_clock .. ' Time of Day', scene.chrono.time, nil, 0.0, 24.0, '%.2f h', true)
        scene.chrono.time_cycle_scale = self:_inspect_float(icons.i_alarm_clock .. ' Time Scale', scene.chrono.time_cycle_scale, nil, 0.0, 1.0, nil, true)
    end
    if ui.CollapsingHeader(icons.i_lamp .. ' Lighting', cpp.ImGuiTreeNodeFlags_DefaultOpen) then
        scene.chrono.sun_latitude = self:_inspect_float(icons.i_ruler_triangle .. ' Skylight Latitude', scene.chrono.sun_latitude, nil, -90.0, 90.0, '%.2f deg', true)
        scene.lighting.sun_color = self:_inspect_vec3_color_rgb(icons.i_sun .. ' Skylight Color', scene.lighting.sun_color)
        scene.lighting.ambient_color = self:_inspect_vec3_color_rgb(icons.i_lightbulb .. ' Ambient Color', scene.lighting.ambient_color)
        scene.lighting.sky_turbidity = self:_inspect_float(icons.i_clouds .. ' Atmospheric Turbidity', scene.lighting.sky_turbidity, nil, 1.8, 10.0, nil, true)
    end
end

-- Inspect the selected entity.
function inspector:_inspect_entity()
    local entity = self.selected_entity
    if not entity or not entity:is_valid() then
        ui.TextUnformatted('No entity selected')
    else
        self:_entity_base_header(entity)
        if entity:has_component(c_transform) then
            self:_inspect_component_transform()
        end
        if entity:has_component(c_camera) then
            self:_inspect_component_camera()
        end
        if entity:has_component(c_mesh_renderer) then
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
    self.name_changed = false
    ui.SetNextWindowSize(utils.default_window_size, cpp.ImGuiCond_FirstUseEver)
    if ui.Begin(self.name, self.is_visible) then
        if self.inspector_mode == 'env_editor' then
            self:_inspect_env_editor()
        elseif self.inspector_mode == 'scene_cfg_editor' then
            -- TODO: Implement scene settings inspector
        else
            self:_inspect_entity()
        end
    end
    ui.End()
end

return inspector
