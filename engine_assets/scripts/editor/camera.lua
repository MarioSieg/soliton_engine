-- Copyright (c) 2023 Mario "Neo" Sieg. All Rights Reserved.
-- Created on 2/12/23.

-- Import engine modules
local app = require 'app'
local input = require 'input'
local gmath = require 'gmath'
local quat = require 'quat'
local time = require 'time'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local c_transform = require 'components.transform'

local abs, rad = math.abs, math.rad

local camera = {
    target_entity = nil,
    sensitivity = 0.5, -- mouse look sensitivity
    clamp_y = 80.0, -- mouse look Y-axis clamp
    default_movement_speed = 4.0, -- default movement speed
    fast_movement_speed = 10.0 * 4.0, -- movement speed when pressing fast movement button (e.g. shift) (see below)

    enable_mouse_look = true, -- enables/disables looking around
    enable_mouse_button_look = true, -- if true looking around is only working while a mouse button is down
    look_mouse_button = input.mouse_buttons.right, -- the mouse button to look around if the above option is true

    enable_movement = true, -- enables/disables camera movement
    movement_keys = { -- the keys to move the camera around
        forward = input.keys.w,
        backward = input.keys.s,
        left = input.keys.a,
        right = input.keys.d,
    },
    enable_fast_movement = true, -- enable faster movement when the key below is pressed
    fast_movement_key = input.keys.left_shift, -- move fast when this key is pressed
    lock_movement_axis = vec3.one, -- enables to disable the movement on any axis, by setting the axis to 0
    enable_smooth_movement = false,
    smooth_movement_time = 0.5,
    enable_smooth_look = true,
    smooth_look_snappiness = 12.0,

    _prev_mous_pos = vec2.zero,
    _mouse_angles = vec2.zero,
    _smooth_angles = vec2.zero,
    _rotation = quat.identity,
    _position = vec3.zero,
    _velocity = vec3.zero,
    _is_focused = true
}

-- invoked every frame
function camera:_update()
    self._is_focused = app.is_focused() and not app.is_any_ui_hovered()
    if not self.target_entity or not self.target_entity:is_valid() then
        eprint('camera has no valid target entity')
    end
    if self.enable_mouse_look and self._is_focused then
        self:_compute_rotation()
    end
    if self.enable_movement then
        self:_compute_position()
    end
end

function camera:_compute_rotation()
    local sens = abs(self.sensitivity) * 0.01
    local clamp_y_rad = rad(abs(self.clamp_y))
    local mouse_pos = input.get_mouse_position()

    local delta = mouse_pos
    delta = delta - self._prev_mous_pos
    self._prev_mous_pos = mouse_pos

    if self.enable_mouse_button_look and not input.is_mouse_button_pressed(self.look_mouse_button) then
        return
    end

    if self.enable_smooth_look then
        local factor = self.smooth_look_snappiness * time.delta_time
        self._smooth_angles.x = gmath.lerp(self._smooth_angles.x, delta.x, factor)
        self._smooth_angles.y = gmath.lerp(self._smooth_angles.y, delta.y, factor)
        delta = self._smooth_angles
    end

    delta = delta * vec2(sens, sens)
    self._mouse_angles = self._mouse_angles + delta
    self._mouse_angles.y = gmath.clamp(self._mouse_angles.y, -clamp_y_rad, clamp_y_rad)
    self._rotation = quat.from_euler(self._mouse_angles.y, self._mouse_angles.x, 0)
    self.target_entity:get_component(c_transform):set_rotation(self._rotation)
end

function camera:_compute_position()
    local delta = time.delta_time
    local speed = abs(self.default_movement_speed)

    if self.enable_fast_movement then
        if input.is_key_pressed(input.keys.left_shift) then -- are we moving fast (sprinting?)
            speed = abs(self.fast_movement_speed)
        end
    end

    local target = self._position

    local function update_pos(dir)
        local move_speed = speed
        if not self.enable_smooth_movement then -- if we use raw movement, we have to apply the delta time here
            move_speed = move_speed * delta
        end
        target = target + (dir * self._rotation) * move_speed
    end

    if self._is_focused then
        if input.is_key_pressed(self.movement_keys.forward) then
            update_pos(vec3.forward)
        end
        if input.is_key_pressed(self.movement_keys.backward) then
            update_pos(vec3.backward)
        end
        if input.is_key_pressed(self.movement_keys.left) then
            update_pos(vec3.left)
        end
        if input.is_key_pressed(self.movement_keys.right) then
            update_pos(vec3.right)
        end
    end

    if self.enable_smooth_movement then -- smooth movement
        local _position, _velocity = vec3.smooth_damp(self._position, target, self._velocity, self.smooth_movement_time, math.huge, delta) -- smooth damp and apply delta time
        self._position = _position
        -- self._velocity = vec3.clamp(self._velocity, -speed, speed)
    else -- raw movement
        self._position = target
    end

    self._position = self._position * self.lock_movement_axis -- apply axis lock
    self.target_entity:get_component(c_transform):set_position(self._position)
end

return camera
