-- Import this file to create a new scene. Rename the file to match the scene name.
-- The scene name is the first argument to Scene.new() below.

-- Import engine modules
local App = require 'App'
local Color = require 'Color'
local Debug = require 'Debug'
local Entity = require 'Entity'
local Math = require 'Math'
local Quat = require 'Quat'
local Scene = require 'Scene'
local Time = require 'Time'
local Tween = require 'Tween'
local Vec2 = require 'Vec2'
local Vec3 = require 'Vec3'

-- Start scene
local function onStart(scene)

    print('Starting scene...')
end

-- Tick scene
local function onTick(scene)

end

Scene.new('Untitled', onStart, onTick)
