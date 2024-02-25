-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.

local App = require 'App'
local Scene = require 'Scene'
local Components = require 'Components'
local Input = require 'Input'
local Math = require 'Math'
local Vec2 = require 'Vec2'
local Vec3 = require 'Vec3'
local Quat = require 'Quat'
local Time = require 'Time'
local EFLAGS = ENTITY_FLAGS

local Player = {
    controller = nil,
    camera = nil,
    sensitivity = 0.5,
    clampY = 80,
    enableSmoothLook = true,
    lookSnappiness = 15.0,
    walkSpeed = 5,
    runSpeed = 10,
    jumpSpeed = 5,
    _isRunning = false,
    _prevMousePos = Vec2.ZERO,
    _mouseAngles = Vec2.ZERO,
    _smoothAngles = Vec2.ZERO
}

function Player:spawn()
    self.controller = Scene.spawn('PlayerController')
    self.controller:addFlag(EFLAGS.TRANSIENT)
    self.controller:getComponent(Components.Transform)
    self.controller:getComponent(Components.CharacterController):setLinearVelocity(Vec3(0, 30, 0))

    self.camera = Scene.spawn('PlayerCamera')
    self.camera:addFlag(EFLAGS.TRANSIENT)
    self.camera:getComponent(Components.Transform)
    self.camera:getComponent(Components.Camera)
end

function Player:updateCamera()
    local transform = self.camera:getComponent(Components.Transform)
    local newPos = self.controller:getComponent(Components.Transform):getPosition()
    newPos.y = newPos.y + (1.35*0.5) - 0.05
    transform:setPosition(newPos) -- sync pos

    local sens = Math.abs(self.sensitivity) * 0.01
    local clampYRad = Math.rad(Math.abs(self.clampY))
    local mousePos = Input.getMousePos()

    local delta = mousePos
    delta = delta - self._prevMousePos
    self._prevMousePos = mousePos

    if self.enableSmoothLook then
        local factor = self.lookSnappiness * Time.deltaTime
        self._smoothAngles.x = Math.lerp(self._smoothAngles.x, delta.x, factor)
        self._smoothAngles.y = Math.lerp(self._smoothAngles.y, delta.y, factor)
        delta = self._smoothAngles
    end

    delta = delta * Vec2(sens, sens)
    self._mouseAngles = self._mouseAngles + delta
    self._mouseAngles.y = Math.clamp(self._mouseAngles.y, -clampYRad, clampYRad)
    local quat = Quat.fromYawPitchRoll(self._mouseAngles.x, self._mouseAngles.y, 0.0)
    transform:setRotation(quat)
end

function Player:updateMovement()
    local cameraTransform = self.camera:getComponent(Components.Transform)
    local controller = self.controller:getComponent(Components.CharacterController)
    local dir = Vec3.ZERO
    self._isRunning = Input.isKeyPressed(Input.KEYS.LEFT_SHIFT)
    local speed = self._isRunning and self.runSpeed or self.walkSpeed
    if Input.isKeyPressed(Input.KEYS.W) then
        dir = dir + cameraTransform:getForwardDir() * speed
    end
    if Input.isKeyPressed(Input.KEYS.S) then
        dir = dir + cameraTransform:getBackwardDir() * speed
    end
    if Input.isKeyPressed(Input.KEYS.A) then
        dir = dir + cameraTransform:getLeftDir() * speed
    end
    if Input.isKeyPressed(Input.KEYS.D) then
        dir = dir + cameraTransform:getRightDir() * speed
    end
    if Input.isKeyPressed(Input.KEYS.SPACE) then
        dir = dir + Vec3(0, self.jumpSpeed, 0)
    end
    
    local groundState = controller:getGroundState()

    --  Cancel movement in opposite direction of normal when touching something we can't walk up
    if groundState == CHARACTER_GROUND_STATE.ON_STEEP_GROUND or groundState == CHARACTER_GROUND_STATE.NOT_SUPPORTED then
        local normal = controller:getGroundNormal()
        local dot = Vec3.dot(normal, dir)
        if dot < 0 then
            dir = dir - (dot * normal) / Vec3.magSqr(normal)
        end
    end

    -- Update velocity
    local current = controller:getLinearVelocity()
    local desired = dir * 2.0
    desired.y = current.y
    local new = 0.75 * current + 0.25 * desired

    -- Jump
    if groundState == CHARACTER_GROUND_STATE.ON_GROUND and Input.isKeyPressed(Input.KEYS.SPACE) then
        new.y = 5.0
    end

    controller:setLinearVelocity(new)
end

function Player:tick()
    self:updateCamera()
    self:updateMovement()
end

function Player:despawn()
    Scene.despawn(self.controller)
    Scene.despawn(self.camera)
    self.controller = nil
    self.camera = nil
end

return Player
