-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local FFI = require 'ffi'
local GUI = require 'imgui.gui'
require 'lu.editor.style'

WINDOW_SIZE = GUI.ImVec2(800+200, 600+200)

local editor = {}
editor.isVisible = FFI.new('bool[1]', true)

local terminal = require 'lu.editor.terminal'
local profiler = require 'lu.editor.profiler'
local scriptEditor = require 'lu.editor.scripteditor'

editor.tools = {
    terminal,
    profiler,
    scriptEditor
}

setupDarkStyle()

function editor:renderMainMenu()
    if GUI.BeginMainMenuBar() then
        if GUI.BeginMenu('File') then
            if GUI.MenuItem('Exit', 'Alt+F4') then
                editor.isVisible[0] = false
            end
            GUI.EndMenu()
        end
        if GUI.BeginMenu('Tools') then
            for _, tool in ipairs(editor.tools) do
                if GUI.MenuItem(tool.name, nil, tool.isVisible[0]) then
                    tool.isVisible[0] = not tool.isVisible[0]
                end
            end
            GUI.EndMenu()
        end
        if GUI.BeginMenu('Help') then
            GUI.EndMenu()
        end
        GUI.Separator()
        GUI.Text(string.format('FPS: %d', Time.fpsAvg))
        local time = os.date('*t')
        GUI.Separator()
        GUI.Text(string.format('%02d:%02d', time.hour, time.min))
        GUI.EndMainMenuBar()
    end
end

function editor:__onTick()
    if editor.isVisible[0] then
        editor:renderMainMenu()
        GUI.DockSpaceOverViewport(GUI.GetMainViewport(), FFI.C.ImGuiDockNodeFlags_PassthruCentralNode)
        for _, tool in ipairs(editor.tools) do
            if tool.isVisible[0] then
                tool:render()
            end
        end
    end
end

return editor
