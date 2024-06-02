-- Copyright (c) 2022-2023 Mario 'Neo' Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

require 'editor.gconsts'

local ffi = require 'ffi'
local bit = require 'bit'
local band = bit.band
local ui = require 'editor.imgui'
local style = require 'editor.style'
local app = require 'app'
local time = require 'time'
local debugdraw = require 'debugdraw'
local vec3 = require 'vec3'
local scene = require 'scene'
local input = require 'input'
local components = require 'components'
local json = require 'json'
local icons = require 'editor.icons'
local project = require 'editor.project'
local terminal = require 'editor.tools.terminal'
local profiler = require 'editor.tools.profiler'
local script_editor = require 'editor.tools.scripteditor'
local entity_list_view = require 'editor.tools.entity_list_view'
local inspector = require 'editor.tools.inspector'
local asset_explorer = require 'editor.tools.asset_explorer'
local host_info = app.host.graphics_api..' | '..(app.host.host_string)
local cpu_name = 'CPU: '..app.host.cpu_name
local gpu_name = 'GPU: '..app.host.gpu_name
local dock_left = 0.6
local dock_right = 0.4
local dock_bottom = 0.5
local popupid_new_project = 0xffffffff-1
local menu_padding = ui.GetStyle().FramePadding
local is_ingame_ui_wireframe_on = false
local new_project_max_math = 512
local new_project_tmp = nil
local is_creating_project = false
local created_project_dir = ''
local entity_flags = entity_flags
local overlay_location = 1 -- Top right is default
local restore_layout_guard = true
local default_project_location = ''
local texture_filter = build_filter_string(TEXTURE_FILE_EXTS)
local mesh_filter = build_filter_string(MESH_FILE_EXTS)
local script_filter = build_filter_string(SCRIPT_FILE_EXTS)
local font_filter = build_filter_string(FONT_FILE_EXTS)
local material_filter = build_filter_string(MATERIAL_FILE_EXTS)
local sound_filter = build_filter_string(SOUND_FILE_EXTS)
local icons_filter = build_filter_string(ICONS_FILE_EXTS)
local xaml_filter = build_filter_string(XAML_FILE_EXTS)
local overlay_flags = ffi.C.ImGuiWindowFlags_NoDecoration
    + ffi.C.ImGuiWindowFlags_AlwaysAutoResize
    + ffi.C.ImGuiWindowFlags_NoSavedSettings
    + ffi.C.ImGuiWindowFlags_NoFocusOnAppearing
    + ffi.C.ImGuiWindowFlags_NoNav
local debug_mode = {
    none = 0,
    scene_aabbs = 1,
    phsics_shapes = 2,
    ui = 3
}
local debug_mode_names = {
    icons.ADJUST..' PBR',
    icons.GLOBE_EUROPE..' Entities',
    icons.CAR..' Physics',
    icons.WINDOW_FRAME..' ui',
}

if jit.os == 'Windows' then
    default_project_location = os.getenv('USERPROFILE')..'/Documents/'
    default_project_location = string.gsub(default_project_location, '\\', '/')
else -- Linux, MacOS
    default_project_location = os.getenv('HOME')..'/Documents/'
end
if not lfs.attributes(default_project_location) then
    default_project_location = ''
end
default_project_location = default_project_location..'lunam_projects/'
if not lfs.attributes(default_project_location) then
    lfs.mkdir(default_project_location)
end

local debug_mode_names_c = ffi.new("const char*[?]", #debug_mode_names)
for i=1, #debug_mode_names do debug_mode_names_c[i-1] = ffi.cast("const char*", debug_mode_names[i]) end
local config_file_name = 'config/editor.json'

local editor = {
    is_visible = true,
    is_ingame = false,
    tools = {
        terminal,
        profiler,
        script_editor,
        entity_list_view,
        inspector,
        asset_explorer,
    },
    gizmos = {
        show_grid = true,
        grid_step = 1.0,
        grid_dims = vec3(512, 0, 512),
        grid_color = vec3(0.8, 0.8, 0.8),
        grid_fade_start = 85,
        grid_fade_end = 25,
        gizmo_obb_color = vec3(0, 1, 0),
        gizmo_operation = debugdraw.gizmo_operation.universal,
        gizmo_mode = debugdraw.gizmo_mode.local_space,
        gizmo_snap = ffi.new('bool[1]', true),
        gizmo_snap_step = ffi.new('float[1]', 0.1),
        active_debug_mode = ffi.new('int[1]', debug_mode.none)
    },
    serialized_config = {
        general = {
            prev_scene_location = '',
            prev_project_location = '',
        }
    },
    camera = require 'editor.camera',
    dock_id = nil,
    active_project = nil,
    show_demo_window = false,
}

for _, tool in ipairs(editor.tools) do
    tool.is_visible[0] = true
end

function editor.gizmos:draw_gizmos()
    debugdraw.start()
    if self.active_debug_mode[0] == debug_mode.scene_aabbs then
        debugdraw.draw_all_aabbs(vec3(0, 1, 0))
    elseif self.active_debug_mode[0] == debug_mode.phsics_shapes then
        debugdraw.draw_all_physics_shapes()
    end
    if self.is_ingame then
        return
    end
    local selected = entity_list_view.selectedEntity
    if selected and selected:is_valid() then
        debugdraw.enable_gizmo(not selected:has_flag(entity_flags.static))
        debugdraw.draw_gizmo_manipulator(selected, self.gizmo_operation, self.gizmo_mode, self.gizmo_snap[0], self.gizmo_snap_step[0], self.gizmo_obb_color)
    end
    if self.show_grid then
        debugdraw.enable_fade(true)
        debugdraw.set_fade_range(self.grid_fade_start, self.grid_fade_start+self.grid_fade_end)
        debugdraw.draw_grid(self.grid_dims, self.grid_step, self.grid_color)
        debugdraw.enable_fade(false)
    end
end

function editor:reset_ui_layout()
    if self.dock_id then
        ui.DockBuilderRemoveNode(self.dock_id)
        ui.DockBuilderAddNode(self.dock_id, ffi.C.ImGuiDockNodeFlags_DockSpace)
        ui.DockBuilderSetNodeSize(self.dock_id, ui.GetMainViewport().Size)
        local dock_main_id = ffi.new('ImGuiID[1]') -- cursed
        dock_main_id[0] = self.dock_id
        local dockid_right = ui.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Right, dock_right, nil, dock_main_id)
        local dockid_bottom = ui.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Down, dock_bottom, nil, dock_main_id)
        local dockid_left = ui.DockBuilderSplitNode(dock_main_id[0], ffi.C.ImGuiDir_Left, dock_left, nil, dock_main_id)
        ui.DockBuilderDockWindow(terminal.name, dockid_bottom)
        ui.DockBuilderDockWindow(profiler.name, dockid_bottom)
        ui.DockBuilderDockWindow(script_editor.name, dockid_bottom)
        ui.DockBuilderDockWindow(asset_explorer.name, dockid_bottom)
        ui.DockBuilderDockWindow(entity_list_view.name, dockid_left)
        ui.DockBuilderDockWindow(inspector.name, dockid_right)
    end
end

function editor:load_scene(file)
    entity_list_view.selectedEntity = nil
    if file then
        scene.load(file)
    else
        scene.new('Untitled scene')
    end
    local main_camera = scene.spawn('__editor_camera__') -- spawn editor camera
    main_camera:add_flag(entity_flags.hidden + entity_flags.transient) -- hide and don't save
    main_camera:get_component(components.camera):set_fov(80)
    self.camera.target_entity = main_camera
    entity_list_view:buildEntityList()
end

local player = require 'editor.player' -- TODO: hacky

function editor:start_game_mode()
    entity_list_view:buildEntityList()
    app.window.enable_cursor(false)
    self.camera.enable_movement = false
    self.camera.enable_mouse_look = false
    local spawnPos = self.camera._position
    spawnPos.y = spawnPos.y + 2.0
    player:spawn(spawnPos)
    scene.set_active_camera_entity(player._camera)
    self.is_visible = false
end

function editor:update_scene()
    player:_update()
end

function editor:stop_game_mode()
    player:despawn()
    scene.set_active_camera_entity(self.camera.target_entity)
    entity_list_view:buildEntityList()
    app.window.enable_cursor(true)
    self.camera.enable_movement = true
    self.camera.enable_mouse_look = true
    self.is_visible = true
end

function editor:toggle_game_mode()
    self.is_ingame = not self.is_ingame
    if self.is_ingame then
        self:start_game_mode()
    else
        self:stop_game_mode()
    end
end

function editor:draw_main_menu_bar()
    ui.PushStyleVar(ffi.C.ImGuiStyleVar_FramePadding, menu_padding)
    if ui.BeginMainMenuBar() then
        ui.PopStyleVar(1)
        if ui.BeginMenu('File') then
            if ui.MenuItem(icons.FOLDER_PLUS..' Create project...') then
                ui.PushOverrideID(popupid_new_project)
                ui.OpenPopup(icons.FOLDER_PLUS..' New project')
                ui.PopID()
            end
            if ui.MenuItem(icons.FOLDER_OPEN..' Open project...') then
                local selected_file = app.utils.open_file_dialog('Lunam Projects', 'lupro', self.serialized_config.general.prev_project_location)
                if selected_file and lfs.attributes(selected_file) then
                    self.serialized_config.general.prev_project_location = selected_file:match("(.*[/\\])")
                    local proj = project:open(selected_file)
                    print('Opened project: '..proj.full_path)
                    if self.active_project then -- unload previous project
                        self.active_project:unload()
                        self.active_project = nil
                    end
                    self.active_project = proj
                    app.window.set_platform_title(string.format('%s', proj.serialized.name))
                    collectgarbage('collect')
                    collectgarbage('stop')
                end
            end
            if ui.MenuItem(icons.PLUS_CIRCLE..' New scene') then
                self:load_scene(nil)
            end
            if ui.MenuItem(icons.FILE_IMPORT..' Open scene') then
                local selected_file = app.utils.open_file_dialog('3D Scenes', mesh_filter, self.serialized_config.general.prev_scene_location)
                if selected_file and lfs.attributes(selected_file) then
                    self.serialized_config.general.prev_scene_location = selected_file:match("(.*[/\\])")
                    self:load_scene(selected_file)
                end
            end
            if ui.MenuItem(icons.PORTAL_EXIT..' Exit') then
                app.exit()
            end
            ui.EndMenu()
        end
        if ui.BeginMenu('Tools') then
            for i=1, #self.tools do
                local tool = self.tools[i]
                if ui.MenuItem(tool.name, nil, tool.is_visible[0]) then
                    tool.is_visible[0] = not tool.is_visible[0]
                end
            end
            ui.EndMenu()
        end
        if ui.BeginMenu('View') then
            if ui.MenuItem('Fullscreen', nil, app.window._is_fullscreen) then
                if app.window._is_fullscreen then
                    app.window.fillscreen_exit()
                else
                    app.window.fillscreen_enter()
                end
            end
            if ui.MenuItem(icons.RULER_TRIANGLE..' Show Grid', nil, self.gizmos.show_grid) then
                self.gizmos.show_grid = not self.gizmos.show_grid
            end
            if ui.MenuItem(icons.ARROW_UP..' Show Center Axis', nil, self.gizmos.showCenterAxis) then
                self.gizmos.showCenterAxis = not self.gizmos.showCenterAxis
            end
            ui.EndMenu()
        end
        if ui.BeginMenu('Help') then
            if ui.MenuItem(icons.BOOK_OPEN..' Open Lua API Documentation') then
                local INDEX = 'docs/lua/index.html'
                if lfs.attributes(INDEX) then
                    if jit.os == 'Windows' then
                        pcall(os.execute('start "" '..'"'..INDEX..'"'))
                    else
                        pcall(os.execute('open '..'"'..INDEX..'"'))
                    end
                else
                    eprint('Lua API documentation not found: '..INDEX)
                end
            end
            if ui.MenuItem(icons.BOOK_OPEN..' Open C++ SDK Documentation') then
                local INDEX = 'docs/html/index.html'
                if lfs.attributes(INDEX) then
                    if jit.os == 'Windows' then
                        pcall(os.execute('start "" '..'"'..INDEX..'"'))
                    else
                        pcall(os.execute('open '..'"'..INDEX..'"'))
                    end
                else
                    eprint('C++ SDK documentation not found: '..INDEX)
                end
            end
            if jit.os ~= 'Windows' then -- Currently only POSIX support
                if ui.MenuItem(icons.COGS..' Regenerate Lua API Documentation') then
                    local GENERATOR = 'gen_lua_docs.sh'
                    if lfs.attributes(GENERATOR) then
                        pcall(os.execute('bash '..'"'..GENERATOR..'" &'))
                    else
                        eprint('Lua API documentation generator not found: '..GENERATOR)
                    end
                end
                if ui.MenuItem(icons.COGS..' Regenerate C++ API Documentation') then
                    local GENERATOR = 'gen_cpp_docs.sh'
                    if lfs.attributes(GENERATOR) then
                        pcall(os.execute('bash '..'"'..GENERATOR..'" &'))
                    else
                        eprint('C++ SDK documentation generator not found: '..GENERATOR)
                    end
                end
            end
            if ui.MenuItem('Perform Full GC Cycle') then
                collectgarbage('collect')
                collectgarbage('stop')
            end
            if ui.MenuItem('Show ui Demo Window', nil, self.show_demo_window) then
                self.show_demo_window = not self.show_demo_window
            end
            ui.EndMenu()
        end
        ui.Separator()
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0)
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_BorderShadow, 0)
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Border, 0)
        if ui.SmallButton(self.gizmos.gizmo_mode == debugdraw.gizmo_mode.local_space and icons.HOUSE or icons.GLOBE) then
            self.gizmos.gizmo_mode = band(self.gizmos.gizmo_mode + 1, 1)
        end
        ui.PopStyleColor(3)
        if ui.IsItemHovered() then
            ui.SetTooltip('Gizmo Mode: '..(self.gizmos.gizmo_mode == debugdraw.gizmo_mode.local_space and 'Local' or 'World'))
        end
        ui.Checkbox(icons.RULER, self.gizmos.gizmo_snap)
        if ui.IsItemHovered() then
            ui.SetTooltip('Enable/Disable Gizmo Snap')
        end
        ui.PushItemWidth(75)
        ui.SliderFloat('##SnapStep', self.gizmos.gizmo_snap_step, 0.1, 5.0, '%.1f', 1.0)
        if ui.IsItemHovered() then
            ui.SetTooltip('Gizmo Snap Step')
        end
        ui.PopItemWidth()
        ui.PushItemWidth(120)
        if ui.Combo('##DebugRenderingMode', self.gizmos.active_debug_mode, debug_mode_names_c, #debug_mode_names) then
            if self.gizmos.active_debug_mode[0] == debug_mode.ui then
                is_ingame_ui_wireframe_on = not is_ingame_ui_wireframe_on
                app.hotReloadUI(is_ingame_ui_wireframe_on)
            else
                is_ingame_ui_wireframe_on = false
                app.hotReloadUI(is_ingame_ui_wireframe_on)
            end
        end
        ui.PopItemWidth()
        if ui.IsItemHovered() then
            ui.SetTooltip('debugdraw rendering mode')
        end
        ui.Separator()
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Button, 0)
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_BorderShadow, 0)
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Border, 0)
        if ui.SmallButton(self.is_ingame and icons.STOP_CIRCLE or icons.PLAY_CIRCLE) then
            self:toggle_game_mode()
        end
        if ui.IsItemHovered() then
            ui.SetTooltip(self.is_ingame and 'Stop' or 'Play scene')
        end
        ui.PopStyleColor(3)
        ui.Separator()
        if ui.Button(icons.FLAME..' ui') then
            app.hotReloadUI()
        end
        if ui.IsItemHovered() then
            ui.SetTooltip('Reload game ui')
        end
        if ui.Button(icons.FLAME..' Shaders') then
            app.hotReloadShaders()
        end
        if ui.IsItemHovered() then
            ui.SetTooltip('Reload shaders')
        end
        ui.Separator()
        ui.Text('FPS: %g', math.floor(time.fps_avg))
        if profiler.isProfilerRunning then
            ui.Separator()
            ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff0000ff)
            ui.TextUnformatted(icons.STOPWATCH)
            ui.PopStyleColor()
            if ui.IsItemHovered() and ui.BeginTooltip() then
                ui.TextUnformatted('profiler is running')
                ui.EndTooltip()
            end
        end
        ui.EndMainMenuBar()
    end
end

function editor:draw_pending_popups()
    ui.PushOverrideID(popupid_new_project)
    if ui.BeginPopupModal(icons.FOLDER_PLUS..' New project') then
        if not new_project_tmp then
            new_project_tmp = ffi.new('char[?]', new_project_max_math)
        end
        if not is_creating_project then
            local defaultName = 'my project'
            created_project_dir = default_project_location..defaultName
            ffi.copy(new_project_tmp, defaultName)
            is_creating_project = true
        end
        if ui.InputText('##ProjName', new_project_tmp, new_project_max_math) then
            created_project_dir = default_project_location..ffi.string(new_project_tmp)
        end
        ui.SameLine()
        if ui.Button('...') then
            local dir = app.utils.open_folder_dialog(nil)
            if dir and lfs.attributes(dir) then
                default_project_location = dir
                created_project_dir = default_project_location..ffi.string(new_project_tmp)
            end
        end
        ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, 0xff999999)
        ui.Text('Final path: '..created_project_dir)
        ui.PopStyleColor()
        if ui.Button('Create ') then
            local name = ffi.string(new_project_tmp)
            local dir = default_project_location
            if dir and name and #name ~= 0 then
                print('Creating project on disk: '..dir)
                local success, err = pcall(function() project:create(dir, name) end)
                if success then
                    ui.CloseCurrentPopup()
                    new_project_tmp[0] = 0
                    is_creating_project = false
                    print('project created: '..dir)
                else
                    eprint('Failed to create project: '..err)
                end
            else
                eprint('Invalid project name or directory')
            end
        end
        ui.SameLine()
        if ui.Button('Cancel') then
            ui.CloseCurrentPopup()
            new_project_tmp[0] = 0
            is_creating_project = false
        end
        ui.EndPopup()
    end
    ui.PopID()
end

function editor:draw_ingame_overlay()
    local overlayFlags = overlay_flags
    if overlay_location >= 0 then
        local PAD = 10.0
        local viewport = ui.GetMainViewport()
        local workPos = viewport.WorkPos
        local workSize = viewport.WorkSize
        local windowPos = ui.ImVec2(0, 0)
        windowPos.x = band(overlay_location, 1) ~= 0 and (workPos.x + workSize.x - PAD) or (workPos.x + PAD)
        windowPos.y = band(overlay_location, 2) ~= 0 and (workPos.y + workSize.y - PAD) or (workPos.y + PAD)
        local windowPosPivot = ui.ImVec2(0, 0)
        windowPosPivot.x = band(overlay_location, 1) ~= 0 and 1.0 or 0.0
        windowPosPivot.y = band(overlay_location, 2) ~= 0 and 1.0 or 0.0
        ui.SetNextWindowPos(windowPos, ffi.C.ImGuiCond_Always, windowPosPivot)
        overlayFlags = overlayFlags + ffi.C.ImGuiWindowFlags_NoMove
    elseif overlay_location == -2 then
        local viewport = ui.GetMainViewport()
        ui.SetNextWindowPos(viewport:GetCenter(), ffi.C.ImGuiCond_Always, ui.ImVec2(0.5, 0.5))
        overlayFlags = overlayFlags - ffi.C.ImGuiWindowFlags_NoMove
    end
    ui.SetNextWindowBgAlpha(0.35)
    if ui.Begin('Overlay', nil, overlayFlags) then
        ui.TextUnformatted(string.format('Sim Hz: %d, T: %.01f, %sT: %f', time.fps_avg, time.time, icons.TRIANGLE, time.delta_time))
        ui.SameLine()
        local size = app.window.get_frame_buffer_size()
        ui.TextUnformatted(string.format(' | %d X %d', size.x, size.y))
        ui.TextUnformatted(string.format('GC Mem: %.03f MB', collectgarbage('count')/1000.0))
        ui.SameLine()
        local time = os.date('*t')
        ui.TextUnformatted(string.format(' | %02d.%02d.%02d %02d:%02d', time.day, time.month, time.year, time.hour, time.min))
        ui.Separator()
        local camera = scene.get_active_camera_entity()
        if camera:is_valid() and camera:has_component(components.transform) then
            local transform = camera:get_component(components.transform)
            ui.TextUnformatted(string.format('Pos: %s', transform:get_position()))
            ui.TextUnformatted(string.format('Dir: %s', transform:get_forward_dir()))
        end
        ui.Separator()
        ui.TextUnformatted(host_info)
        ui.TextUnformatted(cpu_name)
        ui.TextUnformatted(gpu_name)
        if ui.BeginPopupContextWindow() then
            if ui.MenuItem('Custom', nil, overlay_location == -1) then
                overlay_location = -1
            end
            if ui.MenuItem('Center', nil, overlay_location == -2) then
                overlay_location = -2
            end
            if ui.MenuItem('Top-left', nil, overlay_location == 0) then
                overlay_location = 0
            end
            if ui.MenuItem('Top-right', nil, overlay_location == 1) then
                overlay_location = 1
            end
            if ui.MenuItem('Bottom-left', nil, overlay_location == 2) then
                overlay_location = 2
            end
            if ui.MenuItem('Bottom-right', nil, overlay_location == 3) then
                overlay_location = 3
            end
            ui.EndPopup()
        end
    end
    ui.End()
end

function editor:drawTools()
    self.dock_id = ui.DockSpaceOverViewport(ui.GetMainViewport(), ffi.C.ImGuiDockNodeFlags_PassthruCentralNode)
    if restore_layout_guard then
        restore_layout_guard = false
        self:reset_ui_layout()
    end
    for i=1, #self.tools do
        local tool = self.tools[i]
        if tool.is_visible[0] then
            tool:render()
        end
    end
    if self.show_demo_window then 
        ui.ShowDemoWindow()
    end
end

function editor:_update()
    if self.is_ingame then
        self:update_scene()
        self:draw_ingame_overlay()
        if input.is_key_pressed(input.keys.escape) then -- Exit game mode
            self:toggle_game_mode()
        end
    end
    self.gizmos:draw_gizmos()
    if not self.is_visible then
        return
    end
    self.camera:_update()
    local selectedE = entity_list_view.selectedEntity
    if entity_list_view.selectedWantsFocus and selectedE and selectedE:is_valid() then
        if selectedE:has_component(components.transform) then
            local pos = selectedE:get_component(components.transform):get_position()
            pos.z = pos.z - 1.0
            if pos then
                self.camera._position = pos
                -- self.camera._rotation = quat.IDENTITY
            end
        end
        entity_list_view.selectedWantsFocus = false
    end
    inspector.selectedEntity = selectedE
    if inspector.propertiesChanged then
        entity_list_view:buildEntityList()
    end
    self:drawTools()
    self:draw_main_menu_bar()
    self:draw_pending_popups()
end

function editor:loadConfig() -- TODO: Save config on exit
    if lfs.attributes(config_file_name) then
        self.serialized_config = json.deserialize_from_file(config_file_name)
    else
        print('Creating new editor config file: '..config_file_name)
        self:saveConfig()
    end
    if not self.serialized_config.general.prev_project_location or not lfs.attributes(self.serialized_config.general.prev_project_location) then
        self.serialized_config.general.prev_project_location = default_project_location
    end
    if not self.serialized_config.general.prev_scene_location or not lfs.attributes(self.serialized_config.general.prev_scene_location) then
        self.serialized_config.general.prev_scene_location = default_project_location
    end
end

function editor:saveConfig()
    json.serialize_to_file(config_file_name, self.serialized_config)
end

style.setup()

editor:loadConfig()
editor:load_scene(nil)

return editor
