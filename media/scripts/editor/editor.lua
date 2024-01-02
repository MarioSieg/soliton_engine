-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'editor.imgui'
local style = require 'editor.style'

local Terminal = require 'editor.tools.terminal'
local Profiler = require 'editor.tools.profiler'
local ScriptEditor = require 'editor.tools.scripteditor'

WINDOW_SIZE = gui.ImVec2(800, 600)

local Editor = {
    isVisible = ffi.new('bool[1]', true),
    tools = {
        Terminal,
        Profiler,
        ScriptEditor
    },
    gizmos = {
        gridSize = 25,
        showGrid = true,
        showCenterAxis = true
    }
}

for _, tool in ipairs(Editor.tools) do -- hide all tools by default
    tool.isVisible[0] = false
end

style.setupDarkStyle()

function Editor.gizmos:drawGizmos()
    Debug.start()
    if self.showGrid then
        Debug.drawGrid(Debug.AXIS.Y, Vec3.ZERO, self.gridSize, 1)
    end
    if self.showCenterAxis then
        Debug.drawAxis(Vec3.ZERO, 1, Debug.AXIS.Y, 0.02)
    end
    Debug.finish()
end

function Editor:renderMainMenu()
    if gui.BeginMainMenuBar() then
        if gui.BeginMenu('File') then
            if gui.MenuItem(ICONS.PORTAL_EXIT..' Exit', 'Alt+F4') then
                App.exit()
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
            if gui.MenuItem(ICONS.RULER_TRIANGLE..' Show Grid', nil, self.gizmos.showGrid) then
                self.gizmos.showGrid = not self.gizmos.showGrid
            end
            if gui.MenuItem(ICONS.ARROW_UP..' Show Center Axis', nil, self.gizmos.showCenterAxis) then
                self.gizmos.showCenterAxis = not self.gizmos.showCenterAxis
            end
            gui.EndMenu()
        end
        if gui.BeginMenu('Help') then
            gui.EndMenu()
        end
        gui.Separator()
        gui.Text(string.format('FPS: %d', Time.fpsAvg))
        local time = os.date('*t')
        gui.Separator()
        gui.Text(string.format('%02d:%02d', time.hour, time.min))
        if Profiler.isProfilerRunning then
            gui.Separator()
            gui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff0000ff)
            gui.TextUnformatted(ICONS.STOPWATCH)
            gui.PopStyleColor()
            if gui.IsItemHovered() and gui.BeginTooltip() then
                gui.TextUnformatted('Profiler is running')
                gui.EndTooltip()
            end
        end
        gui.EndMainMenuBar()
    end
end

function Editor:drawTools()
    gui.DockSpaceOverViewport(gui.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
    for _, tool in ipairs(self.tools) do
        if tool.isVisible[0] then
            tool:render()
        end
    end
end

function Editor:__onTick()
    if not Editor.isVisible[0] then return end
    self.gizmos:drawGizmos()
    self:drawTools()
    self:renderMainMenu()
end

return Editor
