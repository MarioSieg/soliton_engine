-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
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
local Scene = require 'Scene'
local Components = require 'Components'

local ICONS = require 'editor.icons'
local Project = require 'editor.project'

local Terminal = require 'editor.tools.terminal'
local Profiler = require 'editor.tools.profiler'
local ScriptEditor = require 'editor.tools.scripteditor'
local EntityListView = require 'editor.tools.entity_list_view'
local Inspector = require 'editor.tools.inspector'

WINDOW_SIZE = UI.ImVec2(800, 600)
local DOCK_LEFT_RATIO = 0.50
local DOCK_RIGHT_RATIO = 0.40
local DOCK_BOTTOM_RATIO = 0.50
local MAX_TEXT_INPUT_SIZE = 128
local POPUP_ID_NEW_PROJECT = 0xffffffff-1

local DEFAULT_PROJECT_DIR = ''
if jit.os == 'Windows' then
    DEFAULT_PROJECT_DIR = os.getenv('USERPROFILE')..'/Documents/'
    DEFAULT_PROJECT_DIR = string.gsub(DEFAULT_PROJECT_DIR, '\\', '/')
else -- Linux, MacOS
    DEFAULT_PROJECT_DIR = os.getenv('HOME')..'/Documents/'
end

if not lfs.attributes(DEFAULT_PROJECT_DIR) then
    DEFAULT_PROJECT_DIR = ''
end

DEFAULT_PROJECT_DIR = DEFAULT_PROJECT_DIR..'Lunam Projects/'
if not lfs.attributes(DEFAULT_PROJECT_DIR) then
    lfs.mkdir(DEFAULT_PROJECT_DIR)
end

local MESH_FILE_FILTER = '3d,3ds,3mf,ac,ac3d,acc,amj,ase,ask,b3d,bvh,csm,cob,dae,dxf,enff,fbx,gltf,glb,hmb,ifc,irr,lwo,lws,lxo,m3d,md2,md3,md5,mdc,mdl,mesh,mot,ms3d,ndo,nff,obj,off,ogex,ply,pmx,prj,q3o,q3s,raw,scn,sib,smd,stp,stl,ter,uc,vta,x,x3d,xgl,zgl'

local Editor = {
    isVisible = ffi.new('bool[1]', true),
    isPlaying = false,
    tools = {
        Terminal,
        Profiler,
        ScriptEditor,
        EntityListView,
        Inspector
    },
    gizmos = {
        gridSize = 25,
        showGrid = true,
        showCenterAxis = true
    },
    camera = require 'editor.camera',
    dockID = nil,
    inputTextBuffer = ffi.new('char[?]', 1+MAX_TEXT_INPUT_SIZE),
    activeProject = nil
}

for _, tool in ipairs(Editor.tools) do
    tool.isVisible[0] = true
end

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

function Editor:defaultDockLayout()
    if self.dockID then
        UI.DockBuilderRemoveNode(self.dockID)
        UI.DockBuilderAddNode(self.dockID, ffi.C.ImGuiDockNodeFlags_DockSpace)
        UI.DockBuilderSetNodeSize(self.dockID, UI.GetMainViewport().Size)
        local dock_main_id = ffi.new('ImGuiID[1]') -- cursed
        dock_main_id[0] = self.dockID
        local dockBot = UI.DockBuilderSplitNode(self.dockID, ffi.C.ImGuiDir_Down, DOCK_BOTTOM_RATIO, nil, dock_main_id)
        local dockLeft = UI.DockBuilderSplitNode(self.dockID, ffi.C.ImGuiDir_Left, DOCK_LEFT_RATIO, nil, dock_main_id)
        local dockRight = UI.DockBuilderSplitNode(self.dockID, ffi.C.ImGuiDir_Right, DOCK_RIGHT_RATIO, nil, dock_main_id)
        UI.DockBuilderDockWindow(Terminal.name, dockBot)
        UI.DockBuilderDockWindow(Profiler.name, dockBot)
        UI.DockBuilderDockWindow(ScriptEditor.name, dockBot)
        UI.DockBuilderDockWindow(EntityListView.name, dockLeft)
        UI.DockBuilderDockWindow(Inspector.name, dockRight)
    end
end

function Editor:loadScene(file)
    Scene.load('Default', file)
    local mainCamera = Scene.spawn('__editorCamera') -- spawn editor camera
    mainCamera:addFlag(ENTITY_FLAGS.HIDDEN + ENTITY_FLAGS.TRANSIENT) -- hide and don't save
    mainCamera:component(Components.Camera):setFov(80)
    self.camera.targetEntity = mainCamera
    EntityListView:buildEntityList()
end

function Editor:renderMainMenu()
    if UI.BeginMainMenuBar() then
        if UI.BeginMenu('File') then
            if UI.MenuItem(ICONS.FOLDER_PLUS..' New Project...') then
                UI.PushOverrideID(POPUP_ID_NEW_PROJECT)
                UI.OpenPopup(ICONS.FOLDER_PLUS..' New Project')
                UI.PopID()
            end
            if UI.MenuItem(ICONS.FOLDER_OPEN..' Open Project...') then
                local selectedFile = App.Utils.openFileDialog('Lunam Projects', 'lupro', DEFAULT_PROJECT_DIR)
                if selectedFile and lfs.attributes(selectedFile) then
                    local project = Project:open(selectedFile)
                    print('Opened project: '..project.transientFullPath)
                    self.activeProject = project
                    App.Window.setPlatformTitle(string.format('Project: %s', project.serialized.name))
                    collectgarbage('collect')
                end
            end
            if UI.MenuItem(ICONS.FILE_IMPORT..' Import Scene') then
                local selectedFile = App.Utils.openFileDialog('3D Scenes', MESH_FILE_FILTER, '')
                if selectedFile and lfs.attributes(selectedFile) then
                    Editor:loadScene(selectedFile)
                end
            end
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
            if UI.MenuItem('Perform Full GC Cycle') then
                collectgarbage('collect')
            end
            UI.EndMenu()
        end
        UI.Separator()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, self.isPlaying and 0xff000088 or 0xff008800)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_BorderShadow, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Border, 0)
        if UI.SmallButton(self.isPlaying and ICONS.STOP_CIRCLE or ICONS.PLAY_CIRCLE) then
            self.isPlaying = not self.isPlaying
            if self.isPlaying then

            else

            end
        end
        UI.PopStyleColor(4)
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

local tmpProjName = ''
local tmpDir = ''
function Editor:renderPopups()
    UI.PushOverrideID(POPUP_ID_NEW_PROJECT)
    if UI.BeginPopupModal(ICONS.FOLDER_PLUS..' New Project') then
        if self.inputTextBuffer[0] == 0 then -- Init
            tmpProjName = 'New Project'
            tmpDir = DEFAULT_PROJECT_DIR..tmpProjName
            ffi.copy(self.inputTextBuffer, tmpProjName)
        end
        if UI.InputText('Name', self.inputTextBuffer, MAX_TEXT_INPUT_SIZE) then
            tmpProjName = ffi.string(self.inputTextBuffer)
            tmpDir = DEFAULT_PROJECT_DIR..tmpProjName
        end
        if UI.Button('...') then
            tmpDir = App.Utils.openFolderDialog(nil)..tmpProjName
        end
        UI.SameLine()
        UI.Text(tmpDir)
        if UI.Button('Create ') then
            local name = tmpProjName
            local dir = tmpDir
            if not dir or not name or #name == 0 then
                error('Invalid project name or directory')
            end
            local project = Project:new(dir, name)
            print('Creating project on disk: '..dir)
            if pcall(function() project:createOnDisk(dir) end) then
                print('Project created: '..dir)
            else
                UI.CloseCurrentPopup()
                error('Failed to create project')
            end
            UI.CloseCurrentPopup()
            self.inputTextBuffer[0] = 0
        end
        UI.SameLine()
        if UI.Button('Cancel') then
            UI.CloseCurrentPopup()
            self.inputTextBuffer[0] = 0
        end
        UI.EndPopup()
    end
    UI.PopID()
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
        UI.Text(string.format('Sim Hz: %d, T: %.01f, %sT: %f', Time.fpsAvg, Time.time, ICONS.TRIANGLE, Time.deltaTime))
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
        UI.Separator()
        UI.Text(App.Host.GRAPHICS_API..' | '..App.Host.HOST)
        UI.Text(App.Host.CPU_NAME)
        UI.Text(App.Host.GPU_NAME)
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

local restoreLayout = true
function Editor:drawTools()
    self.dockID = UI.DockSpaceOverViewport(UI.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
    if restoreLayout then
        restoreLayout = false
        self:defaultDockLayout()
    end
    for i=1, #self.tools do
        local tool = self.tools[i]
        if tool.isVisible[0] then
            tool:render()
        end
    end
end

function Editor:__onTick()
    self.camera:tick()
    if not Editor.isVisible[0] then
        return
    end
    Inspector.selectedEntity = EntityListView.selectedEntity
    if Inspector.propertiesChanged then
        EntityListView:buildEntityList()
    end
    self.gizmos:drawGizmos()
    self:drawTools()
    self:renderMainMenu()
    self:renderPopups()
    self:renderOverlay()
end

Style.setupDarkStyle()
Editor:loadScene(nil)

return Editor
