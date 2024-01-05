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

local Camera = {}

Camera.entity = 0
Camera.sensitivity = 0.5 -- mouse look sensitivity
Camera.clampY = 80 -- mouse look Y-axis clamp
Camera.defaultMovementSpeed = 4 -- default movement speed
Camera.fastMovementSpeed = 6 -- movement speed when pressing fast movement button (e.g. shift) (see below)

Camera.enableMouseLook = true -- enables/disables looking around
Camera.enableMouseButtonLook = false -- if true looking around is only working while a mouse button is down
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
Camera.smoothMovementTime = 1
Camera.enableSmoothLook = false
Camera.lookSnappiness = 15.0

Camera._prevMousePos = Vec2.ZERO
Camera._mouseAngles = Vec2.ZERO
Camera._smoothAngles = Vec2.ZERO
Camera._rotation = Quat.ZERO
Camera._position = Vec3.ZERO
Camera._velocity = Vec3.ZERO

-- invoked every frame
function Camera:tick()
    if self.enableMouseLook then -- TODO focus
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
    delta = delta - self._prevMousePos
    self._prevMousePos = mousePos

    if self.enableMouseButtonLook and not Input.isMouseButtonPressed(self.lookMouseButton) then
        return
    end

    if self.enableSmoothLook then
        local factor = self.lookSnappiness * Time.deltaTime
        self._smoothAngles.x = Math.lerp(self._smoothAngles.x, delta.x, factor)
        self._smoothAngles.y = Math.lerp(self._smoothAngles.y, delta.y, factor)
        delta = self._smoothAngles
    end

    delta = delta * Vec2(sens, sens)
    self._mouseAngles = self._mouseAngles + delta
    self._mouseAngles.y = Math.clamp(self._mouseAngles.y, -clampYRad, clampYRad)
    self._rotation = Quat.fromRollPitchYaw(self._mouseAngles.y, self._mouseAngles.x, 0)
    Entity.setRot(self.entity, self._rotation)
end

function Camera:_computeMovement()
    local delta = Time.deltaTime
    local speed = Math.abs(self.defaultMovementSpeed)

    if self.enableFastMovement then
        if Input.isKeyPressed(Input.KEYS.LEFT_SHIFT) then -- are we moving fast (sprinting?)
            speed = Math.abs(self.fastMovementSpeed)
        end
    end

    local target = self._position

    local function computePos(dir)
        local movSpeed = speed
        if not self.enableSmoothMovement then -- if we use raw movement, we have to apply the delta time here
            movSpeed = movSpeed * delta
        end
        target = target + (dir * self._rotation) * movSpeed
    end

    if true then -- TODO focus
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
        local position, velocity = Vec3.smoothDamp(self._position, target, self._velocity, self.smoothMovementTime, Math.INFINITY, delta) -- smooth damp and apply delta time
        self._position = position
        -- self._velocity = Vec3.clamp(self._velocity, -speed, speed)
    else -- raw movement
        self._position = target
    end

    self._position = self._position * self.lockAxisMovement -- apply axis lock
    Entity.setPos(self.entity, self._position)
end

-- Start scene
local function onStart(scene)
    Camera.entity = scene:getEntityByName('MainCamera')
end

-- Tick scene
local function onTick(scene)
	Camera:tick()
end

Scene.new('Untitled', onStart, onTick)
