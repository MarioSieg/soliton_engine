-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'imgui.gui'
require 'lu.editor.style'

WINDOW_SIZE = gui.ImVec2(800+200, 600+200)

editor = {}
editor.isVisible = ffi.new('bool[1]', true)

local terminal = require 'lu.editor.terminal'
local profiler = require 'lu.editor.profiler'
local scriptEditor = require 'lu.editor.script_editor'

editor.tools = {
    terminal,
    profiler,
    scriptEditor
}

setupDarkStyle()

function editor:renderMainMenu()
    if gui.BeginMainMenuBar() then
        if gui.BeginMenu('File') then
            if gui.MenuItem('Exit', 'Alt+F4') then
                self.isVisible[0] = false
            end
            gui.EndMenu()
        end
        if gui.BeginMenu('Tools') then
            for _, tool in ipairs(self.tools) do
                if gui.MenuItem(tool.name, nil, tool.isVisible[0]) then
                    tool.isVisible[0] = not tool.isVisible[0]
                end
            end
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

function editor:__onTick()
    if self.isVisible[0] then
        self:renderMainMenu()
        gui.DockSpaceOverViewport(gui.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
        for _, tool in ipairs(self.tools) do
            if tool.isVisible[0] then
                tool:render()
            end
        end
    end
end

return editor
