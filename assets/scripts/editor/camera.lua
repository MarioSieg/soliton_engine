-- Copyright (c) 2023 Mario "Neo" Sieg. All Rights Reserved.
-- Created on 2/12/23.

-- Import engine modules
local App = require 'App'
local Color = require 'Color'
local Debug = require 'Debug'
local Entity = require 'Entity'
local Input = require 'Input'
local Math = require 'Math'
local Quat = require 'Quat'
local Scene = require 'Scene'
local Time = require 'Time'
local Vec2 = require 'Vec2'
local Vec3 = require 'Vec3'
local Components = require 'Components'

local Camera = {}

Camera.targetEntity = nil
Camera.sensitivity = 0.5 -- mouse look sensitivity
Camera.clampY = 80 -- mouse look Y-axis clamp
Camera.defaultMovementSpeed = 4 -- default movement speed
Camera.fastMovementSpeed = 10*Camera.defaultMovementSpeed-- movement speed when pressing fast movement button (e.g. shift) (see below)

Camera.enableMouseLook = true -- enables/disables looking around
Camera.enableMouseButtonLook = true -- if true looking around is only working while a mouse button is down
Camera.lookMouseButton = Input.MOUSE_BUTTONS.RIGHT -- the mouse button to look around if the above option is true

Camera.enableMovement = true -- enables/disables camera movement
Camera.movementKeys = { -- the keys to move the camera around
    forward = Input.KEYS.W,
    backward = Input.KEYS.S,
    left = Input.KEYS.A,
    right = Input.KEYS.D,
}
Camera.enableFastMovement = true -- enable faster movement when the key below is pressed
Camera.fastMovementKey = Input.KEYS.LEFT_SHIFT -- move fast when this key is pressed
Camera.lockAxisMovement = Vec3.ONE -- enables to disable the movement on any axis, by setting the axis to 0
Camera.enableSmoothMovement = false
Camera.smoothMovementTime = 1.0
Camera.enableSmoothLook = true
Camera.lookSnappiness = 15.0
Camera.prevMousePos = Vec2.ZERO
Camera.mouseAngles = Vec2.ZERO
Camera.smoothAngles = Vec2.ZERO
Camera.rotation = Quat.IDENTITY
Camera.position = Vec3.ZERO
Camera.velocity = Vec3.ZERO
Camera.isFocused = true

-- invoked every frame
function Camera:tick()
    self.isFocused = App.isFocused() and not App.isUIHovered()
    if not self.targetEntity or not self.targetEntity:isValid() then
        perror('Camera has no valid target entity')
    end
    if self.enableMouseLook and self.isFocused then
        self:_computeCameraRotation()
    end
    if self.enableMovement then
        self:_computeMovement()
    end
end

function Camera:_computeCameraRotation()
    local sens = Math.abs(self.sensitivity) * 0.01
    local clampYRad = Math.rad(Math.abs(self.clampY))
    local mousePos = Input.getMousePos()

    local delta = mousePos
    delta = delta - self.prevMousePos
    self.prevMousePos = mousePos

    if self.enableMouseButtonLook and not Input.isMouseButtonPressed(self.lookMouseButton) then
        return
    end

    if self.enableSmoothLook then
        local factor = self.lookSnappiness * Time.deltaTime
        self.smoothAngles.x = Math.lerp(self.smoothAngles.x, delta.x, factor)
        self.smoothAngles.y = Math.lerp(self.smoothAngles.y, delta.y, factor)
        delta = self.smoothAngles
    end

    delta = delta * Vec2(sens, sens)
    self.mouseAngles = self.mouseAngles + delta
    self.mouseAngles.y = Math.clamp(self.mouseAngles.y, -clampYRad, clampYRad)
    self.rotation = Quat.fromYawPitchRoll(self.mouseAngles.x, self.mouseAngles.y, 0.0)
    self.targetEntity:getComponent(Components.Transform):setRotation(self.rotation)
end

function Camera:_computeMovement()
    local delta = Time.deltaTime
    local speed = Math.abs(self.defaultMovementSpeed)

    if self.enableFastMovement then
        if Input.isKeyPressed(Input.KEYS.LEFT_SHIFT) then -- are we moving fast (sprinting?)
            speed = Math.abs(self.fastMovementSpeed)
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
        if Input.isKeyPressed(self.movementKeys.forward) then
            computePos(Vec3.FORWARD)
        end
        if Input.isKeyPressed(self.movementKeys.backward) then
            computePos(Vec3.BACKWARD)
        end
        if Input.isKeyPressed(self.movementKeys.left) then
            computePos(Vec3.LEFT)
        end
        if Input.isKeyPressed(self.movementKeys.right) then
            computePos(Vec3.RIGHT)
        end
    end

    if self.enableSmoothMovement then -- smooth movement
        local position, velocity = Vec3.smoothDamp(self.position, target, self.velocity, self.smoothMovementTime, Math.INFINITY, delta) -- smooth damp and apply delta time
        self.position = position
        -- self._velocity = Vec3.clamp(self._velocity, -speed, speed)
    else -- raw movement
        self.position = target
    end

    self.position = self.position * self.lockAxisMovement -- apply axis lock
    self.targetEntity:getComponent(Components.Transform):setPosition(self.position)
end

return Camera
