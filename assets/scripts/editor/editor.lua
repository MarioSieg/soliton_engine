-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local bit = require 'bit'
local band = bit.band

local UI = require 'editor.imgui'
local Style = require 'editor.style'

require 'editor.gconsts'

local App = require 'App'
local Time = require 'Time'
local Debug = require 'Debug'
local Vec3 = require 'Vec3'
local Quat = require 'Quat'
local Scene = require 'Scene'
local Input = require 'Input'
local Components = require 'Components'
local Ini = require 'Ini'

local ICONS = require 'editor.icons'
local Project = require 'editor.project'

local Terminal = require 'editor.tools.terminal'
local Profiler = require 'editor.tools.profiler'
local ScriptEditor = require 'editor.tools.scripteditor'
local EntityListView = require 'editor.tools.entity_list_view'
local Inspector = require 'editor.tools.inspector'
local AssetExplorer = require 'editor.tools.asset_explorer'

local HOST_INFO = App.Host.GRAPHICS_API..' | '..(App.Host.HOST)
local CPU_NAME = 'CPU: '..App.Host.CPU_NAME
local GPU_NAME = 'GPU: '..App.Host.GPU_NAME
local DOCK_LEFT_RATIO = 0.6
local DOCK_RIGHT_RATIO = 0.4
local DOCK_BOTTOM_RATIO = 0.5
local POPUP_ID_NEW_PROJECT = 0xffffffff-1
local MAIN_MENU_PADDING = UI.GetStyle().FramePadding

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

DEFAULT_PROJECT_DIR = DEFAULT_PROJECT_DIR..'lunam projects/'

local function buildFilterString(items)
    local r = ''
    for k, v in pairs(items) do
        r = r..k..','
    end
    return r
end

local TEXTURE_FILE_FILTER = buildFilterString(TEXTURE_FILE_EXTS)
local MESH_FILE_FILTER = buildFilterString(MESH_FILE_EXTS)
local SCRIPT_FILE_FILTER = buildFilterString(SCRIPT_FILE_EXTS)
local FONT_FILE_FILTER = buildFilterString(FONT_FILE_EXTS)
local MATERIAL_FILE_FILTER = buildFilterString(MATERIAL_FILE_EXTS)
local SOUND_FILE_FILTER = buildFilterString(SOUND_FILE_EXTS)
local ICONS_FILE_FILTER = buildFilterString(ICONS_FILE_EXTS)
local XAML_FILE_FILTER = buildFilterString(XAML_FILE_EXTS)

local EFLAGS = ENTITY_FLAGS
local DEBUG_MODE = {
    NONE = 0,
    SCENE = 1,
    PHYSICS = 2,
    UI = 3
}
local DEBUG_MODE_NAMES = {
    ICONS.ADJUST..' PBR',
    ICONS.GLOBE_EUROPE..' Entities',
    ICONS.CAR..' Physics',
    ICONS.WINDOW_FRAME..' UI',
}
local DEBUG_MODE_NAMES_C = ffi.new("const char*[?]", #DEBUG_MODE_NAMES)
for i=1, #DEBUG_MODE_NAMES do
    DEBUG_MODE_NAMES_C[i-1] = ffi.cast("const char*", DEBUG_MODE_NAMES[i])
end
local CONFIG_FILE = 'config/editor.ini'

local Editor = {
    isVisible = true,
    isPlaying = false,
    tools = {
        Terminal,
        Profiler,
        ScriptEditor,
        EntityListView,
        Inspector,
        AssetExplorer,
    },
    gizmos = {
        showGrid = true,
        gridStep = 1.0,
        gridDims = Vec3(512, 0, 512),
        gridColor = Vec3(0.8, 0.8, 0.8),
        gridFadeStart = 85,
        gridFadeRange = 25,
        gizmoObbColor = Vec3(0, 1, 0),
        gizmoOperation = Debug.GIZMO_OPERATIONS.UNIVERSAL,
        gizmoMode = Debug.GIZMO_MODE.LOCAL,
        gizmoSnap = ffi.new('bool[1]', true),
        gizmoSnapStep = ffi.new('float[1]', 0.1),
        currentDebugMode = ffi.new('int[1]', DEBUG_MODE.NONE)
    },
    serializedConfig = {
        general = {
            prevSceneOpenDir = '',
            prevProjectOpenDir = '',
        }
    },
    camera = require 'editor.camera',
    dockID = nil,
    activeProject = nil,
    showDemoWindow = false,
}

for _, tool in ipairs(Editor.tools) do
    tool.isVisible[0] = true
end

function Editor.gizmos:drawGizmos()
    Debug.start()
    if self.currentDebugMode[0] == DEBUG_MODE.SCENE then
        Debug.drawSceneDebug(Vec3(0, 1, 0))
    elseif self.currentDebugMode[0] == DEBUG_MODE.PHYSICS then
        Debug.drawPhysicsDebug()
    end
    if Editor.isPlaying then
        return
    end
    local selected = EntityListView.selectedEntity
    if selected and selected:isValid() then
        Debug.gizmoEnable(not selected:hasFlag(EFLAGS.STATIC))
        Debug.gizmoManipulator(selected, self.gizmoOperation, self.gizmoMode, self.gizmoSnap[0], self.gizmoSnapStep[0], self.gizmoObbColor)
    end
    if self.showGrid then
        Debug.enableFade(true)
        Debug.setFadeDistance(self.gridFadeStart, self.gridFadeStart+self.gridFadeRange)
        Debug.drawGrid(self.gridDims, self.gridStep, self.gridColor)
        Debug.enableFade(false)
    end
end

function Editor:defaultDockLayout()
    if self.dockID then
        UI.DockBuilderRemoveNode(self.dockID)
        UI.DockBuilderAddNode(self.dockID, ffi.C.ImGuiDockNodeFlags_DockSpace)
        UI.DockBuilderSetNodeSize(self.dockID, UI.GetMainViewport().Size)
        local dock_main_id = ffi.new('ImGuiID[1]') -- cursed
        dock_main_id[0] = self.dockID
        local dockRight = UI.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Right, DOCK_RIGHT_RATIO, nil, dock_main_id)
        local dockBot = UI.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Down, DOCK_BOTTOM_RATIO, nil, dock_main_id)
        local dockLeft = UI.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Left, DOCK_LEFT_RATIO, nil, dock_main_id)
        UI.DockBuilderDockWindow(Terminal.name, dockBot)
        UI.DockBuilderDockWindow(Profiler.name, dockBot)
        UI.DockBuilderDockWindow(ScriptEditor.name, dockBot)
        UI.DockBuilderDockWindow(AssetExplorer.name, dockBot)
        UI.DockBuilderDockWindow(EntityListView.name, dockLeft)
        UI.DockBuilderDockWindow(Inspector.name, dockRight)
    end
end

function Editor:loadScene(file)
    EntityListView.selectedEntity = nil
    if file then
        Scene.load(file)
    else
        Scene.new('Untitled Scene')
    end
    local mainCamera = Scene.spawn('__EditorCamera') -- spawn editor camera
    mainCamera:addFlag(EFLAGS.HIDDEN + EFLAGS.TRANSIENT) -- hide and don't save
    mainCamera:getComponent(Components.Camera):setFov(80)
    self.camera.targetEntity = mainCamera
    EntityListView:buildEntityList()
end

local Player = require 'editor.player'

function Editor:playScene()
    EntityListView:buildEntityList()
    App.Window.enableCursor(false)
    self.camera.enableMovement = false
    self.camera.enableMouseLook = false
    local spawnPos = self.camera.position
    spawnPos.y = spawnPos.y + 2.0
    Player:spawn(spawnPos)
    Scene.setActiveCameraEntity(Player.camera)
    self.isVisible = false
end

function Editor:tickScene()
    Player:tick()
end

function Editor:stopScene()
    Player:despawn()
    Scene.setActiveCameraEntity(self.camera.targetEntity)
    EntityListView:buildEntityList()
    App.Window.enableCursor(true)
    self.camera.enableMovement = true
    self.camera.enableMouseLook = true
    self.isVisible = true
end

function Editor:switchGameMode()
    self.isPlaying = not self.isPlaying
    if self.isPlaying then
        self:playScene()
    else
        self:stopScene()
    end
end

local isUiWireframeEnabled = false

function Editor:renderMainMenu()
    UI.PushStyleVar(ffi.C.ImGuiStyleVar_FramePadding, MAIN_MENU_PADDING)
    if UI.BeginMainMenuBar() then
        UI.PopStyleVar(1)
        if UI.BeginMenu('File') then
            if UI.MenuItem(ICONS.FOLDER_PLUS..' Create Project...') then
                UI.PushOverrideID(POPUP_ID_NEW_PROJECT)
                UI.OpenPopup(ICONS.FOLDER_PLUS..' New Project')
                UI.PopID()
            end
            if UI.MenuItem(ICONS.FOLDER_OPEN..' Open Project...') then
                local selectedFile = App.Utils.openFileDialog('Lunam Projects', 'lupro', self.serializedConfig.general.prevProjectOpenDir)
                if selectedFile and lfs.attributes(selectedFile) then
                    self.serializedConfig.general.prevProjectOpenDir = selectedFile:match("(.*[/\\])")
                    local project = Project:open(selectedFile)
                    print('Opened project: '..project.transientFullPath)
                    self.activeProject = project
                    App.Window.setPlatformTitle(string.format('Project: %s', project.serialized.name))
                    collectgarbage('collect')
                    collectgarbage('stop')
                end
            end
            if UI.MenuItem(ICONS.PLUS_CIRCLE..' New Scene') then
                self:loadScene(nil)
            end
            if UI.MenuItem(ICONS.FILE_IMPORT..' Open Scene') then
                local selectedFile = App.Utils.openFileDialog('3D Scenes', MESH_FILE_FILTER, self.serializedConfig.general.prevSceneOpenDir)
                if selectedFile and lfs.attributes(selectedFile) then
                    self.serializedConfig.general.prevSceneOpenDir = selectedFile:match("(.*[/\\])")
                    self:loadScene(selectedFile)
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
            if UI.MenuItem(ICONS.BOOK_OPEN..' Open Lua API Documentation') then
                local INDEX = 'docs/lua/index.html'
                if lfs.attributes(INDEX) then
                    if jit.os == 'Windows' then
                        pcall(os.execute('start "" '..'"'..INDEX..'"'))
                    else
                        pcall(os.execute('open '..'"'..INDEX..'"'))
                    end
                else
                    perror('Lua API documentation not found: '..INDEX)
                end
            end
            if UI.MenuItem(ICONS.BOOK_OPEN..' Open C++ SDK Documentation') then
                local INDEX = 'docs/html/index.html'
                if lfs.attributes(INDEX) then
                    if jit.os == 'Windows' then
                        pcall(os.execute('start "" '..'"'..INDEX..'"'))
                    else
                        pcall(os.execute('open '..'"'..INDEX..'"'))
                    end
                else
                    perror('C++ SDK documentation not found: '..INDEX)
                end
            end
            if jit.os ~= 'Windows' then -- Currently only POSIX support
                if UI.MenuItem(ICONS.COGS..' Regenerate Lua API Documentation') then
                    local GENERATOR = 'gen_lua_docs.sh'
                    if lfs.attributes(GENERATOR) then
                        pcall(os.execute('bash '..'"'..GENERATOR..'" &'))
                    else
                        perror('Lua API documentation generator not found: '..GENERATOR)
                    end
                end
                if UI.MenuItem(ICONS.COGS..' Regenerate C++ API Documentation') then
                    local GENERATOR = 'gen_cpp_docs.sh'
                    if lfs.attributes(GENERATOR) then
                        pcall(os.execute('bash '..'"'..GENERATOR..'" &'))
                    else
                        perror('C++ SDK documentation generator not found: '..GENERATOR)
                    end
                end
            end
            if UI.MenuItem('Perform Full GC Cycle') then
                collectgarbage('collect')
                collectgarbage('stop')
            end
            if UI.MenuItem('Show UI Demo Window', nil, self.showDemoWindow) then
                self.showDemoWindow = not self.showDemoWindow
            end
            UI.EndMenu()
        end
        UI.Separator()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_BorderShadow, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Border, 0)
        if UI.SmallButton(self.gizmos.gizmoMode == Debug.GIZMO_MODE.LOCAL and ICONS.HOUSE or ICONS.GLOBE) then
            self.gizmos.gizmoMode = band(self.gizmos.gizmoMode + 1, 1)
        end
        UI.PopStyleColor(3)
        if UI.IsItemHovered() then
            UI.SetTooltip('Gizmo Mode: '..(self.gizmos.gizmoMode == Debug.GIZMO_MODE.LOCAL and 'Local' or 'World'))
        end
        UI.Checkbox(ICONS.RULER, self.gizmos.gizmoSnap)
        if UI.IsItemHovered() then
            UI.SetTooltip('Enable/Disable Gizmo Snap')
        end
        UI.PushItemWidth(75)
        UI.SliderFloat('##SnapStep', self.gizmos.gizmoSnapStep, 0.1, 5.0, '%.1f', 1.0)
        if UI.IsItemHovered() then
            UI.SetTooltip('Gizmo Snap Step')
        end
        UI.PopItemWidth()
        UI.PushItemWidth(120)
        if UI.Combo('##DebugRenderingMode', self.gizmos.currentDebugMode, DEBUG_MODE_NAMES_C, #DEBUG_MODE_NAMES) then
            if self.gizmos.currentDebugMode[0] == DEBUG_MODE.UI then
                isUiWireframeEnabled = not isUiWireframeEnabled
                App.hotReloadUI(isUiWireframeEnabled)
            else
                isUiWireframeEnabled = false
                App.hotReloadUI(isUiWireframeEnabled)
            end
        end
        UI.PopItemWidth()
        if UI.IsItemHovered() then
            UI.SetTooltip('Debug rendering mode')
        end
        UI.Separator()
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_BorderShadow, 0)
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Border, 0)
        if UI.SmallButton(self.isPlaying and ICONS.STOP_CIRCLE or ICONS.PLAY_CIRCLE) then
            self:switchGameMode()
        end
        if UI.IsItemHovered() then
            UI.SetTooltip(self.isPlaying and 'Stop' or 'Play Scene')
        end
        UI.PopStyleColor(3)
        UI.Separator()
        if UI.Button(ICONS.FLAME..' UI') then
            App.hotReloadUI()
        end
        if UI.IsItemHovered() then
            UI.SetTooltip('Reload game UI')
        end
        if UI.Button(ICONS.FLAME..' Shaders') then
            App.hotReloadShaders()
        end
        if UI.IsItemHovered() then
            UI.SetTooltip('Reload shaders')
        end
        UI.Separator()
        UI.Text('FPS: %g', math.floor(Time.fpsAvg))
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

local CREATE_PROJECT_MAX_PATH = 128
local createProjectTextBuf = nil
local creatingProject = false
local createdProjectDir = ''
function Editor:renderPopups()
    UI.PushOverrideID(POPUP_ID_NEW_PROJECT)
    if UI.BeginPopupModal(ICONS.FOLDER_PLUS..' New Project') then
        if not createProjectTextBuf then
            createProjectTextBuf = ffi.new('char[?]', CREATE_PROJECT_MAX_PATH)
        end
        if not creatingProject then
            local defaultName = 'new project'
            createdProjectDir = DEFAULT_PROJECT_DIR..defaultName
            ffi.copy(createProjectTextBuf, defaultName)
            creatingProject = true
        end
        if UI.InputText('##ProjName', createProjectTextBuf, CREATE_PROJECT_MAX_PATH) then
            createdProjectDir = DEFAULT_PROJECT_DIR..ffi.string(createProjectTextBuf)
        end
        UI.SameLine()
        if UI.Button('...') then
            local dir = App.Utils.openFolderDialog(nil)
            if dir and lfs.attributes(dir) then
                DEFAULT_PROJECT_DIR = dir
                createdProjectDir = DEFAULT_PROJECT_DIR..ffi.string(createProjectTextBuf)
            end
        end
        UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff999999)
        UI.Text('Final path: '..createdProjectDir)
        UI.PopStyleColor()
        if UI.Button('Create ') then
            local name = ffi.string(createProjectTextBuf)
            local dir = DEFAULT_PROJECT_DIR
            if dir and name and #name ~= 0 then
                local project = Project:new(dir, name)
                print('Creating project on disk: '..dir)
                if pcall(function() project:createOnDisk(dir) end) then
                    print('Project created: '..dir)
                else
                    eprint('Failed to create project')
                end
                UI.CloseCurrentPopup()
                createProjectTextBuf[0] = 0
                creatingProject = false
            else
                eprint('Invalid project name or directory')
            end
        end
        UI.SameLine()
        if UI.Button('Cancel') then
            UI.CloseCurrentPopup()
            createProjectTextBuf[0] = 0
            creatingProject = false
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
        UI.TextUnformatted(string.format('Sim Hz: %d, T: %.01f, %sT: %f', Time.fpsAvg, Time.time, ICONS.TRIANGLE, Time.deltaTime))
        UI.SameLine()
        local size = App.Window.getFrameBufSize()
        UI.TextUnformatted(string.format(' | %d X %d', size.x, size.y))
        UI.TextUnformatted(string.format('GC Mem: %.03f MB', collectgarbage('count')/1000.0))
        UI.SameLine()
        local time = os.date('*t')
        UI.TextUnformatted(string.format(' | %02d.%02d.%02d %02d:%02d', time.day, time.month, time.year, time.hour, time.min))
        UI.Separator()
        local camera = Scene.getActiveCameraEntity()
        if camera:isValid() and camera:hasComponent(Components.Transform) then
            local transform = camera:getComponent(Components.Transform)
            UI.TextUnformatted(string.format('Pos: %s', transform:getPosition()))
            UI.TextUnformatted(string.format('Dir: %s', transform:getForwardDir()))
        end
        UI.Separator()
        UI.TextUnformatted(HOST_INFO)
        UI.TextUnformatted(CPU_NAME)
        UI.TextUnformatted(GPU_NAME)
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
    if self.showDemoWindow then 
        UI.ShowDemoWindow()
    end
end

function Editor:__onTick()
    if self.isPlaying then
        self:tickScene()
        self:renderOverlay()
        if Input.isKeyPressed(Input.KEYS.ESCAPE) then
            self:switchGameMode()
        end
    end
    self.gizmos:drawGizmos()
    if not self.isVisible then
        return
    end
    self.camera:tick()
    local selectedE = EntityListView.selectedEntity
    if EntityListView.selectedWantsFocus and selectedE and selectedE:isValid() then
        if selectedE:hasComponent(Components.Transform) then
            local pos = selectedE:getComponent(Components.Transform):getPosition()
            pos.z = pos.z - 1.0
            if pos then
                self.camera.position = pos
                self.camera.rotation = Quat.IDENTITY
            end
        end
        EntityListView.selectedWantsFocus = false
    end
    Inspector.selectedEntity = selectedE
    if Inspector.propertiesChanged then
        EntityListView:buildEntityList()
    end
    self:drawTools()
    self:renderMainMenu()
    self:renderPopups()
end

function Editor:loadConfig()
    if lfs.attributes(CONFIG_FILE) then
        self.serializedConfig = Ini.deserialize(CONFIG_FILE)
    else
        print('Creating new editor config file: '..CONFIG_FILE)
        self:saveConfig()
    end
    if not lfs.attributes(self.serializedConfig.general.prevProjectOpenDir) then
        self.serializedConfig.general.prevProjectOpenDir = DEFAULT_PROJECT_DIR
    end
    if not lfs.attributes(self.serializedConfig.general.prevSceneOpenDir) then
        self.serializedConfig.general.prevSceneOpenDir = DEFAULT_PROJECT_DIR
    end
end

function Editor:saveConfig()
    Ini.serialize(CONFIG_FILE, self.serializedConfig)
end

Style.setup()

Editor:loadConfig()
Editor:loadScene(nil)

return Editor
