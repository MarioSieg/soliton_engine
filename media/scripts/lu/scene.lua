-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    uint64_t __lu_scene_new(void);
    void __lu_scene_tick(void);
    void __lu_scene_start(void);
]]

scene = {
    active = nil
}

function scene.new(name, setupFunc, startFunc, tickFunc)
    startFunc = startFunc or function() end
    tickFunc = tickFunc or function() end

    assert(type(setupFunc) == 'function', 'setup_callback must be a function')
    assert(type(startFunc) == 'function', 'start_callback must be a function')
    assert(type(tickFunc) == 'function', 'tick_callback must be a function')

    local data = setupFunc() -- call setup callback to get scene-specific data
    assert(type(data) == 'table', 'setup_callback must return a table')

    name = (name and type(name) == 'string') or 'untitled'
    local id = ffi.C.__lu_scene_new() -- create native scene

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

    scene.active = nil -- clear active scene
    collectgarbage() -- collect garbage before starting new scene

    newScene:__onStart() -- call start callback to start playing
    
    scene.active = newScene
    print(string.format('Created new scene: %s, id: %x', name, id))
end
