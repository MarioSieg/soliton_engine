-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local app = require 'app'
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

local Player = {
    mouse_sensitivity = 0.5,
    view_clamp_y = 80,
    enable_smooth_look = true,
    lookSnappiness = 14.0,
    walk_speed = 2,
    run_speed = 5,
    jump_speed = 2,
    enable_view_bob = true,
    view_bob_speed = 10, -- Speed of oscillation
    view_bob_scale = 0.15, -- Magnitude of oscillation
    
    _controller = nil,
    _camera = nil,
    _movementState = movement_state.idle,
    _isInAir = false,
    _prevMousePos = vec2.zero,
    _mouseAngles = vec2.zero,
    _smoothAngles = vec2.zero
}

function Player:spawn(spawn_pos)
    spawn_pos = spawn_pos or vec3(0, 2, 0)

    self._controller = scene.spawn('player_controller')
    self._controller:add_flag(entity_flags.transient)
    self._controller:get_component(components.character_controller)
    self._controller:get_component(components.transform):set_position(spawn_pos)

    self._camera = scene.spawn('player_camera')
    self._camera:add_flag(entity_flags.transient)
    self._camera:get_component(components.transform)
    self._camera:get_component(components.camera):set_fov(85.0)
end

function Player:_update_camera()
    local transform = self._camera:get_component(components.transform)
    local newPos = self._controller:get_component(components.transform):get_position()
    newPos.y = newPos.y + 1.35 * 0.5 -- _camera height
    transform:set_position(newPos) -- sync pos

    local sens = gmath.abs(self.mouse_sensitivity) * 0.01
    local clampYRad = gmath.rad(gmath.abs(self.view_clamp_y))
    local mousePos = input.get_mouse_position()

    local delta = mousePos
    delta = delta - self._prevMousePos
    self._prevMousePos = mousePos

    if self.enable_smooth_look then
        local factor = self.lookSnappiness * time.delta_time
        self._smoothAngles.x = gmath.lerp(self._smoothAngles.x, delta.x, factor)
        self._smoothAngles.y = gmath.lerp(self._smoothAngles.y, delta.y, factor)
        delta = self._smoothAngles
    end

    delta = delta * vec2(sens, sens)
    self._mouseAngles = self._mouseAngles + delta
    self._mouseAngles.y = gmath.clamp(self._mouseAngles.y, -clampYRad, clampYRad)
    local rot = quat.from_yaw_pitch_roll(self._mouseAngles.x, self._mouseAngles.y, 0.0)

    if self._movementState ~= movement_state.idle and self.enable_view_bob then
        local abs_velocity = #self._controller:get_component(components.character_controller):get_linear_velocity()
        local x = gmath.sin(time.time * self.view_bob_speed) * abs_velocity * self.view_bob_scale / 100.0
        local y = gmath.sin(2.0 * time.time * self.view_bob_speed) * abs_velocity * self.view_bob_scale / 400.0
        rot = rot * quat.from_yaw_pitch_roll(x, y, 0)
    end
    
    transform:set_rotation(rot)
end

function Player:_update_movement()
    local cameraTransform = self._camera:get_component(components.transform)
    local _controller = self._controller:get_component(components.character_controller)
    local isRunning = input.is_key_pressed(input.keys.w) and input.is_key_pressed(input.keys.left_shift)
    local speed = isRunning and self.run_speed or self.walk_speed
    local dir = vec3.zero
    local move = function(key, tr_dir)
        if input.is_key_pressed(key) then
            tr_dir.y = 0
            dir = dir + vec3.normalize(tr_dir) * speed
            return true
        end
        return false
    end

    local isMoving = false
    isMoving = isMoving or move(input.keys.w, cameraTransform:get_forward_dir())
    isMoving = isMoving or move(input.keys.s, cameraTransform:get_backward_dir())
    isMoving = isMoving or move(input.keys.a, cameraTransform:get_left_dir())
    isMoving = isMoving or move(input.keys.d, cameraTransform:get_right_dir())

    if isMoving then
        self._movementState = isRunning and movement_state.running or movement_state.walking
    else
        self._movementState = movement_state.idle
    end

    if input.is_key_pressed(input.keys.space) then -- Jump
        dir = dir + vec3(0, self.jump_speed, 0)
    end
    
    local state = _controller:get_ground_state()
    self._isInAir = state == ground_state.flying
    --  Cancel movement in opposite direction of normal when touching something we can't walk up
    if state == ground_state.on_steep_ground or state == ground_state.not_supported then
        local normal = _controller:get_ground_normal()
        local dot = vec3.dot(normal, dir)
        if dot < 0 then
            dir = dir - (dot * normal) / vec3.sqr_magnitude(normal)
        end
    end

    -- Update velocity
    local current = _controller:get_linear_velocity()
    local desired = dir * 2.0
    desired.y = current.y
    local new = 0.75 * current + 0.25 * desired

    -- Jump
    if state == ground_state.on_ground and input.is_key_pressed(input.keys.space) then
        new.y = 5.0
    end

    _controller:set_linear_velocity(new)
end

function Player:_update()
    self:_update_camera()
    self:_update_movement()
end

function Player:despawn()
    scene.despawn(self._controller)
    scene.despawn(self._camera)
    self._controller = nil
    self._camera = nil
end

return Player
