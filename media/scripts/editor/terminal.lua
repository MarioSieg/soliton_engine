-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'editor.imgui'

local m = {
    name = 'Terminal',
    isVisible = ffi.new('bool[1]', true),
    cmdBufLen = 512,
    cmdBuf = ffi.new('char[?]', 512),
    autoScroll = false
}

function m:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(m.name, m.isVisible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if gui.BeginChild('ScrollingRegion', gui.ImVec2(0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, gui.ImVec2(4.0, 1.0))
            for _, record in ipairs(protocol) do
                gui.TextUnformatted(record)
            end
            gui.PopStyleVar()
            if m.autoScroll then
                gui.SetScrollHereY(1.0)
                m.autoScroll = false
            end
            gui.EndChild()
            gui.Separator()
            if gui.InputText('Input', m.cmdBuf, m.cmdBufLen, ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if m.cmdBuf[0] ~= 0 then
                    local command = ffi.string(m.cmdBuf)
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
                    m.cmdBuf[0] = 0 -- Clear buffer by terminating string
                    m.autoScroll = true
                    gui.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
        end
    end
    gui.End()
end

return m
