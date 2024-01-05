-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

ffi.cdef[[
    uint32_t __lu_scene_new(void);
    void __lu_scene_tick(void);
    void __lu_scene_start(void);
    lua_entity_id __lu_scene_spawn_entity(const char* name);
    lua_entity_id __lu_scene_get_entity_by_name(const char* name);
]]

local C = ffi.C

local activeScene = nil
local Scene = {
    name = nil,
    id = 0,
    startCallback = function(scene) end,
    tickCallback = function(scene) end
}

function Scene.getActive()
    return activeScene
end

function Scene:__onStart()
    C.__lu_scene_start()
    self.startCallback(self)
end

function Scene:__onTick()
    self.tickCallback(self)
    C.__lu_scene_tick()
end

function Scene:spawn(name)
    assert(type(name) == 'string')
    return C.__lu_scene_spawn_entity(name)
end

function Scene:getEntityByName(name)
    assert(name ~= nil)
    assert(type(name) == 'string')
    return C.__lu_scene_get_entity_by_name(name)
end

function Scene.new(name, startFunc, tickFunc)
    startFunc = startFunc or function(scene) end
    tickFunc = tickFunc or function(scene) end

    assert(type(startFunc) == 'function', 'startFunc must be a function')
    assert(type(tickFunc) == 'function', 'tickFunc must be a function')

    name = name or 'untitled'
    local id = C.__lu_scene_new() -- create native scene
    assert(type(id) == 'number' and id ~= 0, 'failed to create scene')

    local scene = {}
    setmetatable(scene, {__index = Scene})
    scene.name = name
    scene.id = id
    scene.startCallback = startFunc
    scene.tickCallback = tickFunc

    activeScene = nil -- clear active scene
    scene:__onStart() -- call start callback to start playing
    activeScene = scene
    collectgarbage('collect') -- collect garbage after new scene is created
    print(string.format('Created new scene: %s, id: %x', name, id))
end

Scene.new('Default')

return Scene
