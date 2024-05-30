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
local EFLAGS = ENTITY_FLAGS

local MOVEMENT_STATE = {
    IDLE = 0,
    WALKING = 1,
    RUNNING = 2
}

local Player = {
    controller = nil,
    camera = nil,
    sensitivity = 0.5,
    clampY = 80,
    enableSmoothLook = true,
    lookSnappiness = 15.0,
    walkSpeed = 2,
    runSpeed = 5,
    jumpSpeed = 2,
    _movementState = MOVEMENT_STATE.IDLE,
    _isInAir = false,
    _prevMousePos = vec2.ZERO,
    _mouseAngles = vec2.ZERO,
    _smoothAngles = vec2.ZERO
}

function Player:spawn(spawnPos)
    spawnPos = spawnPos or vec3(0, 10, 0)

    self.controller = scene.spawn('PlayerController')
    self.controller:addFlag(EFLAGS.TRANSIENT)
    self.controller:getComponent(components.character_controller)
    self.controller:getComponent(components.transform):setPosition(spawnPos)

    self.camera = scene.spawn('PlayerCamera')
    self.camera:addFlag(EFLAGS.TRANSIENT)
    self.camera:getComponent(components.transform)
    self.camera:getComponent(components.camera):setFov(75)
end

function Player:updateCamera()
    local transform = self.camera:getComponent(components.transform)
    local newPos = self.controller:getComponent(components.transform):getPosition()
    newPos.y = newPos.y + 1.35 * 0.5 -- camera height
    transform:setPosition(newPos) -- sync pos

    local sens = gmath.abs(self.sensitivity) * 0.01
    local clampYRad = gmath.rad(gmath.abs(self.clampY))
    local mousePos = input.getMousePos()

    local delta = mousePos
    delta = delta - self._prevMousePos
    self._prevMousePos = mousePos

    if self.enableSmoothLook then
        local factor = self.lookSnappiness * time.deltaTime
        self._smoothAngles.x = gmath.lerp(self._smoothAngles.x, delta.x, factor)
        self._smoothAngles.y = gmath.lerp(self._smoothAngles.y, delta.y, factor)
        delta = self._smoothAngles
    end

    delta = delta * vec2(sens, sens)
    self._mouseAngles = self._mouseAngles + delta
    self._mouseAngles.y = gmath.clamp(self._mouseAngles.y, -clampYRad, clampYRad)
    local quat = quat.fromYawPitchRoll(self._mouseAngles.x, self._mouseAngles.y, 0.0)
    
    if self._movementState ~= MOVEMENT_STATE.IDLE then
        local walkFreq = 9.85
        local walkAmplitude = 0.01
        local runFreq = 15
        local runApmlitude = 0.02
        local freq = time.time * self._movementState == MOVEMENT_STATE.RUNNING and runFreq or walkFreq
        local amplitude = self._movementState == MOVEMENT_STATE.RUNNING and runApmlitude or walkAmplitude
        local bobX = amplitude * gmath.sin(freq)
        local bobY = amplitude * gmath.cos(freq)
        local bobQ = quat.fromYawPitchRoll(0, bobY, 0)
        quat = quat * bobQ
    end
    
    transform:setRotation(quat)
end

function Player:updateMovement()
    local cameraTransform = self.camera:getComponent(components.transform)
    local controller = self.controller:getComponent(components.character_controller)
    local isRunning = input.isKeyPressed(input.KEYS.W) and input.isKeyPressed(input.KEYS.LEFT_SHIFT)
    local speed = isRunning and self.runSpeed or self.walkSpeed
    local dir = vec3.ZERO
    local move = function(key, tr_dir)
        if input.isKeyPressed(key) then
            tr_dir.y = 0
            dir = dir + vec3.norm(tr_dir) * speed
            return true
        end
        return false
    end

    local isMoving = false
    isMoving = isMoving or move(input.KEYS.W, cameraTransform:getForwardDir())
    isMoving = isMoving or move(input.KEYS.S, cameraTransform:getBackwardDir())
    isMoving = isMoving or move(input.KEYS.A, cameraTransform:getLeftDir())
    isMoving = isMoving or move(input.KEYS.D, cameraTransform:getRightDir())

    if isMoving then
        self._movementState = isRunning and MOVEMENT_STATE.RUNNING or MOVEMENT_STATE.WALKING
    else
        self._movementState = MOVEMENT_STATE.IDLE
    end

    if input.isKeyPressed(input.KEYS.SPACE) then -- Jump
        dir = dir + vec3(0, self.jumpSpeed, 0)
    end
    
    local groundState = controller:getGroundState()
    self._isInAir = groundState == CHARACTER_GROUND_STATE.IN_AIR
    --  Cancel movement in opposite direction of normal when touching something we can't walk up
    if groundState == CHARACTER_GROUND_STATE.ON_STEEP_GROUND or groundState == CHARACTER_GROUND_STATE.NOT_SUPPORTED then
        local normal = controller:getGroundNormal()
        local dot = vec3.dot(normal, dir)
        if dot < 0 then
            dir = dir - (dot * normal) / vec3.magSqr(normal)
        end
    end

    -- Update velocity
    local current = controller:getLinearVelocity()
    local desired = dir * 2.0
    desired.y = current.y
    local new = 0.75 * current + 0.25 * desired

    -- Jump
    if groundState == CHARACTER_GROUND_STATE.ON_GROUND and input.isKeyPressed(input.KEYS.SPACE) then
        new.y = 5.0
    end

    controller:setLinearVelocity(new)
end

function Player:tick()
    self:updateCamera()
    self:updateMovement()
end

function Player:despawn()
    scene.despawn(self.controller)
    scene.despawn(self.camera)
    self.controller = nil
    self.camera = nil
end

return Player
