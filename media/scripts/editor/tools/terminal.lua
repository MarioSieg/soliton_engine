-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.imgui'

local terminal = {
    name = ICONS.LINE_COLUMNS..' Terminal',
    isVisible = ffi.new('bool[1]', true),
    cmdBufLen = 512,
    cmdBuf = ffi.new('char[?]', 512),
    autoScroll = ffi.new('bool[1]', true),
}

function terminal:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(self.name, self.isVisible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if gui.BeginChild('ScrollingRegion', gui.ImVec2(0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, gui.ImVec2(4.0, 1.0))
            for _, record in ipairs(protocol) do
                gui.TextUnformatted(record)
                gui.Separator()
            end
            gui.PopStyleVar()
            if self.autoScroll[0] then
                gui.SetScrollHereY(1.0)
                self.autoScroll[0] = false
            end
            gui.EndChild()
            gui.Separator()
            gui.TextUnformatted(string.format('%s %d', ICONS.ENVELOPE, #protocol))
            gui.SameLine()
            if gui.InputTextEx(ICONS.ARROW_LEFT, 'Enter command...', self.cmdBuf, self.cmdBufLen, gui.ImVec2(0, 0), ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if self.cmdBuf[0] ~= 0 then
                    local command = ffi.string(self.cmdBuf)
                    -- split by spaces get first word:
                    local args = {}
                    for word in command:gmatch("%w+") do -- split by spaces
                        table.insert(args, word)
                    end
                    local cmd = args[1] -- get first word
                    if TERMINAL_COMMANDS[cmd] then -- check if command exists
                        TERMINAL_COMMANDS[cmd].execute(args) -- execute command
                    else -- command not found
                        print('Unknown command: '..cmd)
                    end
                    self.cmdBuf[0] = 0 -- Clear buffer by terminating string
                    self.autoScroll[0] = true
                    gui.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            gui.SameLine()
            gui.Checkbox(ICONS.MOUSE..' Scroll', self.autoScroll)
        end
    end
    gui.End()
end

return terminal
