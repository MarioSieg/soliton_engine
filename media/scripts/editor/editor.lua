-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'editor.imgui'
local terminal = require 'editor.terminal'
local profiler = require 'editor.profiler'
local scriptEditor = require 'editor.scripteditor'
local style = require 'editor.style'
local dd = debugdraw

WINDOW_SIZE = gui.ImVec2(800, 600)

local m = {
    isVisible = ffi.new('bool[1]', true),
    tools = {
        terminal,
        profiler,
        scriptEditor
    },
    gizmos = {
        gridSize = 25,
        showGrid = true,
        showCenterAxis = true
    }
}

for _, tool in ipairs(m.tools) do -- hide all tools by default
    tool.isVisible[0] = false
end

style.setupDarkStyle()

function m.gizmos:drawGizmos()
    dd.start()
    if self.showGrid then
        dd.drawGrid(dd.AXIS.Y, vec3.ZERO, self.gridSize, 1)
    end
    if self.showCenterAxis then
        dd.drawAxis(vec3.ZERO, 1, dd.AXIS.Y, 0.02)
    end
    dd.finish()
end

function m:renderMainMenu()
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
        if gui.BeginMenu('View') then
            if gui.MenuItem('Show Grid', nil, self.gizmos.showGrid) then
                self.gizmos.showGrid = not self.gizmos.showGrid
            end
            if gui.MenuItem('Show Center Axis', nil, self.gizmos.showCenterAxis) then
                self.gizmos.showCenterAxis = not self.gizmos.showCenterAxis
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

function m:drawTools()
    gui.DockSpaceOverViewport(gui.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
    for _, tool in ipairs(self.tools) do
        if tool.isVisible[0] then
            tool:render()
        end
    end
end

function m:__onTick()
    if not self.isVisible[0] then return end
    self.gizmos:drawGizmos()
    self:drawTools()
    self:renderMainMenu()
end

return m
