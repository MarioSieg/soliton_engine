local Scene = require 'Scene'

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
