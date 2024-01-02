-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ICONS = require 'editor.icons'
local UI = require 'editor.imgui'

local COMMANDS = {}

COMMANDS['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        App.panic(args[2] or 'Panic!')
    end
}

local Terminal = {
    name = ICONS.LINE_COLUMNS..' Terminal',
    isVisible = ffi.new('bool[1]', true),
    cmdBufLen = 512,
    cmdBuf = ffi.new('char[?]', 512),
    autoScroll = ffi.new('bool[1]', true),
}

function Terminal:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(Terminal.name, Terminal.isVisible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if UI.BeginChild('ScrollingRegion', UI.ImVec2(0, -UI.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            UI.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, UI.ImVec2(4.0, 1.0))
            for _, record in ipairs(protocol) do
                UI.TextUnformatted(record)
                UI.Separator()
            end
            UI.PopStyleVar()
            if Terminal.autoScroll[0] then
                UI.SetScrollHereY(1.0)
                Terminal.autoScroll[0] = false
            end
            UI.EndChild()
            UI.Separator()
            UI.TextUnformatted(string.format('%s %d', ICONS.ENVELOPE, #protocol))
            UI.SameLine()
            if UI.InputTextEx(ICONS.ARROW_LEFT, 'Enter command...', Terminal.cmdBuf, Terminal.cmdBufLen, UI.ImVec2(0, 0), ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if Terminal.cmdBuf[0] ~= 0 then
                    local command = ffi.string(Terminal.cmdBuf)
                    -- split by spaces get first word:
                    local args = {}
                    for word in command:gmatch("%w+") do -- split by spaces
                        table.insert(args, word)
                    end
                    local cmd = args[1] -- get first word
                    if COMMANDS[cmd] then -- check if command exists
                        COMMANDS[cmd].execute(args) -- execute command
                    else -- command not found
                        print('Unknown command: '..cmd)
                    end
                    Terminal.cmdBuf[0] = 0 -- Clear buffer by terminating string
                    Terminal.autoScroll[0] = true
                    UI.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            UI.SameLine()
            UI.Checkbox(ICONS.MOUSE..' Scroll', Terminal.autoScroll)
        end
    end
    UI.End()
end

return Terminal
