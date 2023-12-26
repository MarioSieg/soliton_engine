-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'editor.imgui'
local terminal = require 'editor.terminal'
local profiler = require 'editor.profiler'
local scriptEditor = require 'editor.scripteditor'
local style = require 'editor.style'

WINDOW_SIZE = gui.ImVec2(800+200, 600+200)

local m = {
    isVisible = ffi.new('bool[1]', true),
    tools = {
        terminal,
        profiler,
        scriptEditor
    }
}

style.setupDarkStyle()

function m:renderMainMenu()
    if gui.BeginMainMenuBar() then
        if gui.BeginMenu('File') then
            if gui.MenuItem('Exit', 'Alt+F4') then
                m.isVisible[0] = false
            end
            gui.EndMenu()
        end
        if gui.BeginMenu('Tools') then
            for _, tool in ipairs(m.tools) do
                if gui.MenuItem(tool.name, nil, tool.isVisible[0]) then
                    tool.isVisible[0] = not tool.isVisible[0]
                end
            end
            gui.EndMenu()
        end
        if gui.BeginMenu('Help') then
            gui.EndMenu()
        end
        gui.Separator()
        gui.Text(string.format('FPS: %d', time.fpsAvg))
        local time = os.date('*t')
        gui.Separator()
        gui.Text(string.format('%02d:%02d', time.hour, time.min))
        gui.EndMainMenuBar()
    end
end

function m:__onTick()
    if m.isVisible[0] then
        m:renderMainMenu()
        gui.DockSpaceOverViewport(gui.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
        for _, tool in ipairs(m.tools) do
            if tool.isVisible[0] then
                tool:render()
            end
        end
    end
end

return m
