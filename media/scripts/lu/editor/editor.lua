-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'imgui.gui'

local WINDOW_SIZE = gui.ImVec2(800+200, 600+200)

local terminal = {
    name = 'Terminal',
    is_visible = ffi.new('bool[1]', true),
    command_buf_len = 512,
    command_buf = ffi.new('char[?]', 512),
    scroll_to_bottom = false,
    commands = require 'lu.editor.commands'
}

function terminal:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin('Terminal', self.is_visible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if gui.BeginChild('ScrollingRegion', gui.ImVec2(0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, gui.ImVec2(4.0, 1.0))
            for _, record in ipairs(protocol) do
                gui.TextUnformatted(record)
            end
            gui.PopStyleVar()
            if self.scroll_to_bottom then
                gui.SetScrollHereY(1.0)
                self.scroll_to_bottom = false
            end
            gui.EndChild()
            gui.Separator()
            if gui.InputText('Input', self.command_buf, self.command_buf_len, ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if self.command_buf[0] ~= 0 then
                    local command = ffi.string(self.command_buf)
                    -- split by spaces get first word:
                    local args = {}
                    for word in command:gmatch("%w+") do -- split by spaces
                        table.insert(args, word)
                    end
                    local cmd = args[1] -- get first word
                    if self.commands[cmd] then -- check if command exists
                        self.commands[cmd].execute(args) -- execute command
                    else -- command not found
                        print('Unknown command: '..cmd)
                    end
                    self.command_buf[0] = 0 -- Clear buffer by terminating string
                    self.scroll_to_bottom = true
                    gui.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
        end
    end
    gui.End()
end

local profiler = {
    name = 'Profiler',
    is_visible = ffi.new('bool[1]', true),
    frame_times = ffi.new('float[?]', time.fps_histogram_samples),
}

function profiler:render()
    if gui.Begin('Profiler', self.is_visible) then
        for i = 1, time.fps_histogram_samples do -- copy fps histogram to frame times
            self.frame_times[i-1] = time.fps_histogram[i] or 0.0
        end
        local plot_size = gui.ImVec2(WINDOW_SIZE.x, 200.0)
        gui.PlotHistogram_FloatPtr('Frame times', self.frame_times, time.fps_histogram_samples, 0, nil, 0.0, time.fps_avg * 2.0, plot_size)
        gui.Text(string.format('FPS: %d', time.fps_avg))
        gui.Text(string.format('FPS avg: %d', time.fps_avg))
        gui.Text(string.format('FPS min: %d', time.fps_min))
        gui.Text(string.format('FPS max: %d', time.fps_max))
        gui.Text(string.format('Time: %.3f s', time.time))
        gui.Text(string.format('Delta time: %.3f s', time.delta_time))
        gui.Text(string.format('Frame time: %.3f ms', time.frame_time))
    end
    gui.End()
end

editor = {}
editor.show_editor = ffi.new('bool[1]', true)
editor.tools = {
    terminal,
    profiler
}

function editor:main_menu()
    if gui.BeginMainMenuBar() then
        if gui.BeginMenu('File') then
            gui.Separator()
            if gui.MenuItem('Exit', 'Alt+F4') then
                self.show_editor[0] = false
            end
            gui.EndMenu()
        end
        if gui.BeginMenu('Tools') then
            for _, tool in ipairs(self.tools) do
                if gui.MenuItem(tool.name, nil, tool.is_visible[0]) then
                    tool.is_visible[0] = not tool.is_visible[0]
                end
            end
            gui.EndMenu()
        end
        gui.Separator()
        gui.Text(string.format('FPS: %d', time.fps_avg))
        gui.EndMainMenuBar()
    end
end

function editor:_on_tick_()
    if self.show_editor[0] then
        self:main_menu()
        for _, tool in ipairs(self.tools) do
            if tool.is_visible[0] then
                tool:render()
            end
        end
    end
end

return editor
