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
    name = 'untitled',
    id = 0
}

function Scene.getActive()
    return activeScene
end

function Scene.__onStart()
    C.__lu_scene_start()
end

function Scene.__onTick()
    C.__lu_scene_tick()
end

function Scene.spawn(name)
    return C.__lu_scene_spawn_entity(name)
end

function Scene.getEntityByName(name)
    return C.__lu_scene_get_entity_by_name(name)
end

function Scene.new(name)
    Scene.name = name or 'untitled'
    local id = C.__lu_scene_new() -- create native scene
    assert(type(id) == 'number' and id ~= 0, 'failed to create scene')
    Scene.id = id
    Scene.__onStart() -- invoke start hook
    collectgarbage('collect') -- collect garbage after new scene is created
    print(string.format('Created new scene: %s, id: %x', name, id))
end

Scene.new()

return Scene
