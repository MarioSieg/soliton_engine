-- Import this file to create a new scene. Rename the file to match the scene name.
-- The scene name is the first argument to Scene.new() below.

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

-- Start scene
local function onStart(scene)
	local cam = scene:getEntityByName('MainCamera')
	Entity.setPos(cam, Vec3(0, 6, -20))
    print('Starting scene...')
end

local said = false

-- Tick scene
local function onTick(scene)
	local N = 10
	local NN = 0
	Debug.start()
	for i = 0, N*2 do
		for j = 0, N*2 do
			for k = 0, N*2 do
				local color = Vec3(i/N, k/N, j/N)
				Debug.setColor(color)
				local y = (Math.abs(Math.sin(Time.time))*0.1*k*j)-N*0.5
				Debug.drawSphere(Vec3(i-N*0.5, y, j-N*0.5), 0.2)
				NN = NN + 1
			end
		end
	end
	if not said then
		print(string.format('%d spheres', NN))
		said = true
	end
	Debug.finish()
end

Scene.new('Untitled', onStart, onTick)
