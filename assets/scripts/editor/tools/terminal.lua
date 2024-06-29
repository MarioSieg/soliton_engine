-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local icons = require 'imgui.icons'
local ui = require 'imgui.imgui'

local commands = {}

commands['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        app.panic(args[2] or 'Panic!')
    end
}

local terminal = {
    name = icons.i_line_columns .. ' terminal',
    is_visible = ffi.new('bool[1]', true),
    cmd_buf_len = 512 - 1,
    cmd_buf = ffi.new('char[?]', 512),
    scroll_flags = ffi.new('bool[2]', true, false),
}

function terminal:render()
    ui.SetNextWindowSize(default_window_size, ffi.C.ImGuiCond_FirstUseEver)
    local isLuaLogTab = false
    local protocol = protocol
    if ui.Begin(self.name, self.is_visible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if ui.BeginTabBar('TerminalTabBar', ffi.C.ImGuiTabBarFlags_None) then
            if ui.BeginTabItem(icons.i_code .. ' Lua') then
                isLuaLogTab = true
                local footer = ui.GetStyle().ItemSpacing.y + ui.GetFrameHeightWithSpacing()
                if ui.BeginChild('TerminalScrollingRegion', ui.ImVec2(0, -footer), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
                    ui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, ui.ImVec2(4.0, 1.0))
                    local clipper = ui.ImGuiListClipper()
                    clipper:Begin(#protocol, ui.GetTextLineHeightWithSpacing())
                    while clipper:Step() do -- HOT LOOP
                        for i=clipper.DisplayStart+1, clipper.DisplayEnd do
                            local isError = protocol[i][2]
                            ui.PushStyleColor_U32(ffi.C.ImGuiCol_Text, isError and 0xff4444ff or 0xffffffff)
                            ui.Separator()
                            ui.TextUnformatted(protocol[i][1])
                            ui.PopStyleColor()
                        end
                    end
                    clipper:End()
                    ui.PopStyleVar()
                    if self.scroll_flags[1] then
                        ui.SetScrollHereY(1.0)
                        self.scroll_flags[1] = false
                    end
                    ui.EndChild()
                end
                ui.EndTabItem()
            end
            if ui.BeginTabItem(icons.i_cogs .. ' System') then
                ffi.C.__lu_dd_draw_native_log(self.scroll_flags[1])
                if self.scroll_flags[1] then
                    self.scroll_flags[1] = false
                end
                ui.EndTabItem()
            end
            ui.Separator()
            if ui.InputTextEx(icons.i_arrow_left, 'Enter command...', self.cmd_buf, self.cmd_buf_len, ui.ImVec2(0, 0), ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if self.cmd_buf[0] ~= 0 then
                    local command = ffi.string(self.cmd_buf)
                    -- split by spaces get first word:
                    local args = {}
                    for word in command:gmatch("%w+") do -- split by spaces
                        table.insert(args, word)
                    end
                    local cmd = args[1] -- get first word
                    if commands[cmd] then -- check if command exists
                        commands[cmd].execute(args) -- execute command
                    else -- command not found
                        print('Unknown command: '..cmd)
                    end
                    self.cmd_buf[0] = 0 -- Clear buffer by terminating string
                    self.scroll_flags[1] = true
                    ui.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            ui.SameLine()
            ui.Checkbox(icons.i_mouse .. ' Scroll', self.scroll_flags)
            if self.scroll_flags[0] then
                self.scroll_flags[1] = true
            end
            if isLuaLogTab then
                ui.SameLine()
                if ui.Button(icons.i_trash .. ' Clear') then
                    PROTOCOL = {}
                end
            end
            ui.EndTabBar()
        end
    end
    ui.End()
end

return terminal
