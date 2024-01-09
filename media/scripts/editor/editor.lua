-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local bit = require 'bit'
local band = bit.band

local UI = require 'editor.imgui'
local Style = require 'editor.style'

local App = require 'App'
local Time = require 'Time'
local Debug = require 'Debug'
local Vec3 = require 'Vec3'
local Quat = require 'Quat'

local ICONS = require 'editor.icons'
local Terminal = require 'editor.tools.terminal'
local Profiler = require 'editor.tools.profiler'
local ScriptEditor = require 'editor.tools.scripteditor'
local Camera = require 'editor.camera'
Camera._position = Vec3(0, 1, -6)

WINDOW_SIZE = UI.ImVec2(800, 600)

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
    },
    camera = Camera
}

for _, tool in ipairs(Editor.tools) do -- hide all tools by default
    tool.isVisible[0] = false
end

Style.setupDarkStyle()

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
    if UI.BeginMainMenuBar() then
        if UI.BeginMenu('File') then
            if UI.MenuItem(ICONS.PORTAL_EXIT..' Exit') then
                App.exit()
            end
            UI.EndMenu()
        end
        if UI.BeginMenu('Tools') then
            for i=1, #self.tools do
                local tool = self.tools[i]
                if UI.MenuItem(tool.name, nil, tool.isVisible[0]) then
                    tool.isVisible[0] = not tool.isVisible[0]
                end
            end
            UI.EndMenu()
        end
        if UI.BeginMenu('View') then
            if UI.MenuItem('Fullscreen', nil, App.Window.isFullscreen) then
                if App.Window.isFullscreen then
                    App.Window.leaveFullscreen()
                else
                    App.Window.enterFullscreen()
                end
            end
            if UI.MenuItem(ICONS.RULER_TRIANGLE..' Show Grid', nil, self.gizmos.showGrid) then
                self.gizmos.showGrid = not self.gizmos.showGrid
            end
            if UI.MenuItem(ICONS.ARROW_UP..' Show Center Axis', nil, self.gizmos.showCenterAxis) then
                self.gizmos.showCenterAxis = not self.gizmos.showCenterAxis
            end
            UI.EndMenu()
        end
        if UI.BeginMenu('Help') then
            UI.EndMenu()
        end
        if Profiler.isProfilerRunning then
            UI.Separator()
            UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff0000ff)
            UI.TextUnformatted(ICONS.STOPWATCH)
            UI.PopStyleColor()
            if UI.IsItemHovered() and UI.BeginTooltip() then
                UI.TextUnformatted('Profiler is running')
                UI.EndTooltip()
            end
        end
        UI.EndMainMenuBar()
    end
end

local overlayLocation = 1 -- Top right is default
local FLAG_BASE = ffi.C.ImGuiWindowFlags_NoDecoration
    + ffi.C.ImGuiWindowFlags_AlwaysAutoResize
    + ffi.C.ImGuiWindowFlags_NoSavedSettings
    + ffi.C.ImGuiWindowFlags_NoFocusOnAppearing
    + ffi.C.ImGuiWindowFlags_NoNav
function Editor:renderOverlay()
    local overlayFlags = FLAG_BASE
    if overlayLocation >= 0 then
        local PAD = 10.0
        local viewport = UI.GetMainViewport()
        local workPos = viewport.WorkPos
        local workSize = viewport.WorkSize
        local windowPos = UI.ImVec2(0, 0)
        windowPos.x = band(overlayLocation, 1) ~= 0 and (workPos.x + workSize.x - PAD) or (workPos.x + PAD)
        windowPos.y = band(overlayLocation, 2) ~= 0 and (workPos.y + workSize.y - PAD) or (workPos.y + PAD)
        local windowPosPivot = UI.ImVec2(0, 0)
        windowPosPivot.x = band(overlayLocation, 1) ~= 0 and 1.0 or 0.0
        windowPosPivot.y = band(overlayLocation, 2) ~= 0 and 1.0 or 0.0
        UI.SetNextWindowPos(windowPos, ffi.C.ImGuiCond_Always, windowPosPivot)
        overlayFlags = overlayFlags + ffi.C.ImGuiWindowFlags_NoMove
    elseif overlayLocation == -2 then
        local viewport = UI.GetMainViewport()
        UI.SetNextWindowPos(viewport:GetCenter(), ffi.C.ImGuiCond_Always, UI.ImVec2(0.5, 0.5))
        overlayFlags = overlayFlags - ffi.C.ImGuiWindowFlags_NoMove
    end
    UI.SetNextWindowBgAlpha(0.35)
    if UI.Begin('Overlay', nil, overlayFlags) then
        UI.Text(string.format('FPS: %d, T: %.01f, %sT: %f', Time.fpsAvg, Time.time, ICONS.TRIANGLE, Time.deltaTime))
        UI.SameLine()
        local size = App.Window.getFrameBufSize()
        UI.Text(string.format(' | %d X %d', size.x, size.y))
        UI.Text(string.format('GC Mem: %.03f MB', collectgarbage('count')/1000.0))
        UI.SameLine()
        local time = os.date('*t')
        UI.Text(string.format(' | %02d.%02d.%02d %02d:%02d', time.day, time.month, time.year, time.hour, time.min))
        UI.Separator()
        UI.Text(string.format('Pos: %s', self.camera._position))
        UI.Text(string.format('Rot: %s', self.camera._rotation))
        if UI.BeginPopupContextWindow() then
            if UI.MenuItem('Custom', nil, overlayLocation == -1) then
                overlayLocation = -1
            end
            if UI.MenuItem('Center', nil, overlayLocation == -2) then
                overlayLocation = -2
            end
            if UI.MenuItem('Top-left', nil, overlayLocation == 0) then
                overlayLocation = 0
            end
            if UI.MenuItem('Top-right', nil, overlayLocation == 1) then
                overlayLocation = 1
            end
            if UI.MenuItem('Bottom-left', nil, overlayLocation == 2) then
                overlayLocation = 2
            end
            if UI.MenuItem('Bottom-right', nil, overlayLocation == 3) then
                overlayLocation = 3
            end
            UI.EndPopup()
        end
    end
    UI.End()
end

function Editor:drawTools()
    UI.DockSpaceOverViewport(UI.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
    for i=1, #self.tools do
        local tool = self.tools[i]
        if tool.isVisible[0] then
            tool:render()
        end
    end
end

function Editor:__onTick()
    self.camera:tick()
    if not Editor.isVisible[0] then return end
    self.gizmos:drawGizmos()
    self:drawTools()
    self:renderMainMenu()
    self:renderOverlay()
end

return Editor
