-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local scene = require 'scene'
local components = require 'components'
local input = require 'input'
local gmath = require 'gmath'
local vec2 = require 'vec2'
local vec3 = require 'vec3'
local quat = require 'quat'
local time = require 'time'
local entity_flags = entity_flags

local movement_state = {
    idle = 0,
    walking = 1,
    running = 2
}

local player = {
    camera_fov = 70,
    mouse_sensitivity = 0.5,
    view_clamp_y = 85,
    enable_smooth_look = true,
    look_snappiness = 14.0,
    walk_speed = 1.8,
    run_speed = 3.5,
    jump_speed = 5.0,
    enable_view_bob = true,
    view_bob_speed = 9, -- Speed of oscillation
    view_bob_scale = 0.15, -- Magnitude of oscillation
    can_jump = true,
    can_run = true,
    
    _controller = nil,
    _camera = nil,
    _movement_state = movement_state.idle,
    _is_flying = false,
    _prev_mouse_pos = vec2.zero,
    _mouse_angles = vec2.zero,
    _smooth_angles = vec2.zero
}

function player:spawn(spawn_pos)
    spawn_pos = spawn_pos or vec3(0, 2, 0)

    self._controller = scene.spawn('player_controller')
    self._controller:add_flag(entity_flags.transient)
    self._controller:get_component(components.character_controller)
    self._controller:get_component(components.transform):set_position(spawn_pos)

    self._camera = scene.spawn('player_camera')
    self._camera:add_flag(entity_flags.transient)
    self._camera:get_component(components.transform)
    self._camera:get_component(components.camera):set_fov(self.camera_fov)
end

function player:_update_camera()
    local transform = self._camera:get_component(components.transform)
    local fixed_pos = self._controller:get_component(components.transform):get_position()
    fixed_pos.y = fixed_pos.y + 1.35 * 0.5 -- _camera height
    transform:set_position(fixed_pos) -- sync pos

    local sens = math.abs(self.mouse_sensitivity) * 0.01
    local clamp_y_rad = math.rad(math.abs(self.view_clamp_y))
    local mouse_pos = input.get_mouse_position()

    local delta = mouse_pos
    delta = delta - self._prev_mouse_pos
    self._prev_mouse_pos = mouse_pos

    if self.enable_smooth_look then
        local factor = self.look_snappiness * time.delta_time
        self._smooth_angles.x = gmath.lerp(self._smooth_angles.x, delta.x, factor)
        self._smooth_angles.y = gmath.lerp(self._smooth_angles.y, delta.y, factor)
        delta = self._smooth_angles
    end

    delta = delta * vec2(sens, sens)
    self._mouse_angles = self._mouse_angles + delta
    self._mouse_angles.y = gmath.clamp(self._mouse_angles.y, -clamp_y_rad, clamp_y_rad)
    local rot = quat.from_euler(self._mouse_angles.y, self._mouse_angles.x, 0.0)

    if self._movement_state ~= movement_state.idle and not self._is_flying and self.enable_view_bob then
        local abs_velocity = #self._controller:get_component(components.character_controller):get_linear_velocity()
        local x = math.sin(time.time * self.view_bob_speed) * abs_velocity * self.view_bob_scale / 100.0
        local y = math.sin(2.0 * time.time * self.view_bob_speed) * abs_velocity * self.view_bob_scale / 400.0
        rot = rot * quat.from_euler(y, x, 0)
    end
    
    transform:set_rotation(rot)
end

function player:_update_movement()
    local cam_transform = self._camera:get_component(components.transform)
    local controller = self._controller:get_component(components.character_controller)
    local is_running = self.can_run and input.is_key_pressed(input.keys.w) and input.is_key_pressed(input.keys.left_shift)
    local speed = is_running and self.run_speed or self.walk_speed

    local dir = vec3.zero
    local is_moving = false

    if input.is_key_pressed(input.keys.w) then --  Forward
        local unit_dir = cam_transform:get_forward_dir()
        unit_dir.y = 0.0
        dir = dir + vec3.normalize(unit_dir) * speed
        is_moving = true
    end
    if input.is_key_pressed(input.keys.s) then --  Backward
        local unit_dir = cam_transform:get_backward_dir()
        unit_dir.y = 0.0
        dir = dir + vec3.normalize(unit_dir) * speed
        is_moving = true
    end
    if input.is_key_pressed(input.keys.a) then --  Left
        local unit_dir = cam_transform:get_left_dir()
        unit_dir.y = 0.0
        dir = dir + vec3.normalize(unit_dir) * speed
        is_moving = true
    end
    if input.is_key_pressed(input.keys.d) then --  Right
        local unit_dir = cam_transform:get_right_dir()
        unit_dir.y = 0.0
        dir = dir + vec3.normalize(unit_dir) * speed
        is_moving = true
    end

    if is_moving then
        self._movement_state = is_running and movement_state.running or movement_state.walking
    else
        self._movement_state = movement_state.idle
    end
    
    local state = controller:get_ground_state()
    self._is_flying = state == ground_state.flying
    
    --  Cancel movement in opposite direction of normal when touching something we can't walk up
    if state == ground_state.on_steep_ground or state == ground_state.not_supported then
        local normal = controller:get_ground_normal()
        local dot = vec3.dot(normal, dir)
        if dot < 0.0 then
            dir = dir - (dot * normal) / vec3.sqr_magnitude(normal)
        end
    end

    -- Update velocity
    local current_velocity = controller:get_linear_velocity()
    local desired = dir * 2.0
    desired.y = current_velocity.y
    local target_velocity = 0.75 * current_velocity + 0.25 * desired

    -- Jump
    if self.can_jump and state == ground_state.on_ground and input.is_key_pressed(input.keys.space) then
        target_velocity.y = self.jump_speed
    end

    controller:set_linear_velocity(target_velocity)
end

function player:update()
    self:_update_camera()
    self:_update_movement()
end

function player:despawn()
    self._controller:despawn()
    self._camera:despawn()
    self._controller = nil
    self._camera = nil
end

return player
