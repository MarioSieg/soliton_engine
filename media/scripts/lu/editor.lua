-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.
-- This file implements the editor GUI.
-- The ImGui LuaJIT bindings are useable but somewhat dirty, which makes this file a bit messy - but hey it works!

local ffi = require 'ffi'
local gui = require 'editor.gui'

local terminal = {
    name = 'Terminal',
    visible = ffi.new('bool[1]', true),
    command_buf_len = 512,
    command_buf = ffi.new('char[?]', 512),
    scroll_to_bottom = false,
    commands = require 'editor.commands'
}

function terminal:render()
    gui.SetNextWindowSize(ffi.new('ImVec2', 800+200, 600+200), ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin('Terminal', self.visible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if gui.BeginChild('ScrollingRegion', ffi.new('ImVec2', 0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, ffi.new('ImVec2', 4, 1))
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

editor = {}
editor.show_editor = ffi.new('bool[1]', true)
editor.tools = {
    terminal
}

local function main_menu()
    if gui.BeginMainMenuBar() then
        if gui.BeginMenu('File') then
            if gui.MenuItem('New', 'Ctrl+N') then
                print('New')
            end
            if gui.MenuItem('Open', 'Ctrl+O') then
                print('Open')
            end
            if gui.MenuItem('Save', 'Ctrl+S') then
                print('Save')
            end
            if gui.MenuItem('Save As..', 'Ctrl+Shift+S') then
                print('Save As..')
            end
            gui.Separator()
            if gui.MenuItem('Exit', 'Alt+F4') then
                m.show_editor[0] = false
            end
            gui.EndMenu()
        end
        gui.Separator()
        gui.Text(string.format('FPS: %d', time.fps_avg))
        gui.EndMainMenuBar()
    end
end

function editor._tick_()
    if editor.show_editor[0] then
        main_menu()
        for _, tool in ipairs(editor.tools) do
            tool:render()
        end
    end
end

return editor
