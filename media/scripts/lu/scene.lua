-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    uint32_t __lu_scene_new(void);
    void __lu_scene_tick(void);
    void __lu_scene_start(void);
]]

local Scene = {
    active = nil
}

function Scene.new(name, setupFunc, startFunc, tickFunc)
    startFunc = startFunc or function() end
    tickFunc = tickFunc or function() end

    assert(type(setupFunc) == 'function', 'setupFunc must be a function')
    assert(type(startFunc) == 'function', 'startFunc must be a function')
    assert(type(tickFunc) == 'function', 'tickFunc must be a function')

    local data = setupFunc() -- call setup callback to get scene-specific data
    assert(type(data) == 'table', 'setupFunc must return a table')

    name = name or 'untitled'
    local id = ffi.C.__lu_scene_new() -- create native scene
    assert(type(id) == 'number' and id ~= 0, 'failed to create scene')

    local newScene = {
        name = name,
        id = id,
        data = data,
        startCallback = startFunc,
        tickCallback = tickFunc
    }

    function newScene:__onStart()
        newScene.startCallback(newScene.data)
        ffi.C.__lu_scene_start()
    end

    function newScene:__onTick()
        newScene.tickCallback(newScene.data)
        ffi.C.__lu_scene_tick()
    end

    Scene.active = nil -- clear active scene
    collectgarbage('collect') -- collect garbage before starting new scene

    newScene:__onStart() -- call start callback to start playing
    Scene.active = newScene
    print(string.format('Created new scene: %s, id: %x', name, id))
    App.Window.setTitle(string.format('Lunam Engine v.%s - %s %s - %s.scene', App.engineVersion, jit.os, jit.arch, name))
end

return Scene
