-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local Entity = require 'lu.entity'

ffi.cdef[[
    uint32_t __lu_scene_new(const char* name, const char* gltf_file, double scale);
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
    return Entity:new(C.__lu_scene_spawn_entity(name))
end

function Scene.getEntityByName(name)
    return Entity:new(C.__lu_scene_get_entity_by_name(name))
end

function Scene.new(scene_name, gltf_file, scale)
    assert(scene_name or gltf_file, 'scene name or gltf file must be provided')
    if gltf_file and type(gltf_file) == 'string' then
        if not lfs.attributes(gltf_file) then
            panic(string.format('gltf file %s does not exist', gltf_file))
        end
        scene_name = scene_name or gltf_file:match("[^/]*.gltf$") -- TODO fix
    else
        scene_name = scene_name or 'untitled'
        gltf_file = ''
    end
    Scene.name = scene_name
    assert(type(scene_name) == 'string')
    local id = C.__lu_scene_new(scene_name, gltf_file, scale) -- create native scene
    assert(type(id) == 'number' and id ~= 0, 'failed to create scene')
    Scene.id = id
    Scene.__onStart() -- invoke start hook
    collectgarbage('collect') -- collect garbage after new scene is created
    print(string.format('Created new scene: %s, id: %x', scene_name, id))
end

Scene.new('EmeraldSquare', '/home/neo/Documents/AssetLibrary/A_Bistro/BistroFull.gltf', 0.025)

return Scene
