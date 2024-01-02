-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    uint32_t __lu_scene_new(void);
    void __lu_scene_tick(void);
    void __lu_scene_start(void);
    uint64_t __lu_scene_spawn_entity(const char* name);
]]

local Scene = {
    active = nil
}

function Scene.new(name, setupFunc, startFunc, tickFunc)
    setupFunc = setupFunc or function() return {} end
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

    local SceneInstance = {
        name = name,
        id = id,
        data = data,
        startCallback = startFunc,
        tickCallback = tickFunc
    }

    function SceneInstance:__onStart()
        SceneInstance.startCallback(SceneInstance.data)
        ffi.C.__lu_scene_start()
    end

    function SceneInstance:__onTick()
        SceneInstance.tickCallback(SceneInstance.data)
        ffi.C.__lu_scene_tick()
    end

    function SceneInstance:spawnEntity(name)
        assert(type(name) == 'string', 'name must be a string')
        return ffi.C.__lu_scene_spawn_entity(name)
    end

    Scene.active = nil -- clear active scene
    collectgarbage('collect') -- collect garbage before starting new scene

    SceneInstance:__onStart() -- call start callback to start playing
    Scene.active = SceneInstance
    print(string.format('Created new scene: %s, id: %x', name, id))
end

Scene.new('Default')

return Scene
