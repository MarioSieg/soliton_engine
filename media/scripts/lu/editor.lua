-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.gui'

local m = {}

m.show_editor = ffi.new('bool[1]', true)

local function mainMenu()
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
        gui.Text('FPS: '..tostring(time.avg_fps))
        gui.EndMainMenuBar()
    end
end

function m._tick_()
    if gui.IsKeyPressed_Bool(ffi.C.ImGuiKey_P, false) then -- P
        m.show_editor[0] = not m.show_editor[0]
    end
    if m.show_editor[0] then
        mainMenu()
    end
end

return m
