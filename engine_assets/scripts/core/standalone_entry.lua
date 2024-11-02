-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local app = require 'app'
local vec3 = require 'vec3'
local scene = require 'scene'

local player = require 'editor.player'

function _start()
    player:spawn()
    scene.set_active_camera_entity(player.camera)
    app.window.enable_cursor(false)
end

function _update()
    player:update()
end
