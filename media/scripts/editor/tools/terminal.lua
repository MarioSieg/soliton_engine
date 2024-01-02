-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.imgui'

local Terminal = {
    name = ICONS.LINE_COLUMNS..' Terminal',
    isVisible = ffi.new('bool[1]', true),
    cmdBufLen = 512,
    cmdBuf = ffi.new('char[?]', 512),
    autoScroll = ffi.new('bool[1]', true),
}

function Terminal:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(Terminal.name, Terminal.isVisible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if gui.BeginChild('ScrollingRegion', gui.ImVec2(0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, gui.ImVec2(4.0, 1.0))
            for _, record in ipairs(protocol) do
                gui.TextUnformatted(record)
                gui.Separator()
            end
            gui.PopStyleVar()
            if Terminal.autoScroll[0] then
                gui.SetScrollHereY(1.0)
                Terminal.autoScroll[0] = false
            end
            gui.EndChild()
            gui.Separator()
            gui.TextUnformatted(string.format('%s %d', ICONS.ENVELOPE, #protocol))
            gui.SameLine()
            if gui.InputTextEx(ICONS.ARROW_LEFT, 'Enter command...', Terminal.cmdBuf, Terminal.cmdBufLen, gui.ImVec2(0, 0), ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if Terminal.cmdBuf[0] ~= 0 then
                    local command = ffi.string(Terminal.cmdBuf)
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
                    Terminal.cmdBuf[0] = 0 -- Clear buffer by terminating string
                    Terminal.autoScroll[0] = true
                    gui.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            gui.SameLine()
            gui.Checkbox(ICONS.MOUSE..' Scroll', Terminal.autoScroll)
        end
    end
    gui.End()
end

return Terminal
