-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local app = require 'app'
local scene = require 'scene'
local player = require 'editor.player'

scene.load('engine_assets/meshes/test.gltf')

local redirect = {}

function redirect._start()
    app.window.enable_cursor(false)
    player:spawn()
    scene.set_active_camera_entity(player._camera)
end

function redirect._update()
    player:update()
end

return redirect
