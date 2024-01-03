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

-- Setup initial scene data and return scene storage table
local function _onSetup()
    print('Setting up scene...')
    return {
        -- Add scene-specific data here. Can be accessed from other callbacks via 'scene.store'
    }
end

-- Start scene
local function _onStart(scene)

    print('Starting scene...')
end

-- Tick scene
local function _onTick(scene)

end

Scene.new('Untitled', _onSetup, _onStart, _onTick)
