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
local components = require 'components'

local camera = {}

camera.targetEntity = nil
camera.sensitivity = 0.5 -- mouse look sensitivity
camera.clampY = 80 -- mouse look Y-axis clamp
camera.defaultMovementSpeed = 4 -- default movement speed
camera.fastMovementSpeed = 10*camera.defaultMovementSpeed-- movement speed when pressing fast movement button (e.g. shift) (see below)

camera.enableMouseLook = true -- enables/disables looking around
camera.enableMouseButtonLook = true -- if true looking around is only working while a mouse button is down
camera.lookMouseButton = input.MOUSE_BUTTONS.RIGHT -- the mouse button to look around if the above option is true

camera.enableMovement = true -- enables/disables camera movement
camera.movementKeys = { -- the keys to move the camera around
    forward = input.KEYS.W,
    backward = input.KEYS.S,
    left = input.KEYS.A,
    right = input.KEYS.D,
}
camera.enableFastMovement = true -- enable faster movement when the key below is pressed
camera.fastMovementKey = input.KEYS.LEFT_SHIFT -- move fast when this key is pressed
camera.lockAxisMovement = vec3.ONE -- enables to disable the movement on any axis, by setting the axis to 0
camera.enableSmoothMovement = false
camera.smoothMovementTime = 1.0
camera.enableSmoothLook = true
camera.lookSnappiness = 15.0
camera.prevMousePos = vec2.ZERO
camera.mouseAngles = vec2.ZERO
camera.smoothAngles = vec2.ZERO
camera.rotation = quat.IDENTITY
camera.position = vec3.ZERO
camera.velocity = vec3.ZERO
camera.isFocused = true

-- invoked every frame
function camera:tick()
    self.isFocused = app.is_focused() and not app.is_any_ui_hovered()
    if not self.targetEntity or not self.targetEntity:is_valid() then
        perror('camera has no valid target entity')
    end
    if self.enableMouseLook and self.isFocused then
        self:_computeCameraRotation()
    end
    if self.enableMovement then
        self:_computeMovement()
    end
end

function camera:_computeCameraRotation()
    local sens = gmath.abs(self.sensitivity) * 0.01
    local clampYRad = gmath.rad(gmath.abs(self.clampY))
    local mousePos = input.getMousePos()

    local delta = mousePos
    delta = delta - self.prevMousePos
    self.prevMousePos = mousePos

    if self.enableMouseButtonLook and not input.isMouseButtonPressed(self.lookMouseButton) then
        return
    end

    if self.enableSmoothLook then
        local factor = self.lookSnappiness * time.deltaTime
        self.smoothAngles.x = gmath.lerp(self.smoothAngles.x, delta.x, factor)
        self.smoothAngles.y = gmath.lerp(self.smoothAngles.y, delta.y, factor)
        delta = self.smoothAngles
    end

    delta = delta * vec2(sens, sens)
    self.mouseAngles = self.mouseAngles + delta
    self.mouseAngles.y = gmath.clamp(self.mouseAngles.y, -clampYRad, clampYRad)
    self.rotation = quat.fromYawPitchRoll(self.mouseAngles.x, self.mouseAngles.y, 0.0)
    self.targetEntity:get_component(components.transform):set_rotation(self.rotation)
end

function camera:_computeMovement()
    local delta = time.deltaTime
    local speed = gmath.abs(self.defaultMovementSpeed)

    if self.enableFastMovement then
        if input.isKeyPressed(input.KEYS.LEFT_SHIFT) then -- are we moving fast (sprinting?)
            speed = gmath.abs(self.fastMovementSpeed)
        end
    end

    local target = self.position

    local function computePos(dir)
        local movSpeed = speed
        if not self.enableSmoothMovement then -- if we use raw movement, we have to apply the delta time here
            movSpeed = movSpeed * delta
        end
        target = target + (dir * self.rotation) * movSpeed
    end

    if self.isFocused then
        if input.isKeyPressed(self.movementKeys.forward) then
            computePos(vec3.FORWARD)
        end
        if input.isKeyPressed(self.movementKeys.backward) then
            computePos(vec3.BACKWARD)
        end
        if input.isKeyPressed(self.movementKeys.left) then
            computePos(vec3.LEFT)
        end
        if input.isKeyPressed(self.movementKeys.right) then
            computePos(vec3.RIGHT)
        end
    end

    if self.enableSmoothMovement then -- smooth movement
        local position, velocity = vec3.smoothDamp(self.position, target, self.velocity, self.smoothMovementTime, gmath.INFINITY, delta) -- smooth damp and apply delta time
        self.position = position
        -- self._velocity = vec3.clamp(self._velocity, -speed, speed)
    else -- raw movement
        self.position = target
    end

    self.position = self.position * self.lockAxisMovement -- apply axis lock
    self.targetEntity:get_component(components.transform):set_position(self.position)
end

return camera
