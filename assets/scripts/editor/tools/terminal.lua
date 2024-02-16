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
    mustAutoScroll = true,
    autoScrollOn = ffi.new('bool[1]', true),
}

function Terminal:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if UI.Begin(self.name, self.isVisible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if UI.BeginChild('TerminalScrollingRegion', UI.ImVec2(0, -UI.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            UI.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, UI.ImVec2(4.0, 1.0))
            local clipper = UI.ImGuiListClipper()
            clipper:Begin(#protocol, UI.GetTextLineHeightWithSpacing())
            while clipper:Step() do -- HOT LOOP
                for i=clipper.DisplayStart+1, clipper.DisplayEnd do
                    UI.TextUnformatted(protocol[i])
                    UI.Separator()
                end
            end
            clipper:End()
            UI.PopStyleVar()
            if self.mustAutoScroll then
                UI.SetScrollHereY(1.0)
                self.mustAutoScroll = false
            end
            UI.EndChild()
            UI.Separator()
            UI.TextUnformatted(string.format('%s %d', ICONS.ENVELOPE, #protocol))
            UI.SameLine()
            if UI.InputTextEx(ICONS.ARROW_LEFT, 'Enter command...', self.cmdBuf, self.cmdBufLen, UI.ImVec2(0, 0), ffi.C.ImGuiInputTextFlags_EnterReturnsTrue) then
                if self.cmdBuf[0] ~= 0 then
                    local command = ffi.string(self.cmdBuf)
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
                    self.cmdBuf[0] = 0 -- Clear buffer by terminating string
                    self.mustAutoScroll = true
                    UI.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            UI.SameLine()
            UI.Checkbox(ICONS.MOUSE..' Scroll', self.autoScrollOn)
            if self.autoScrollOn[0] then
                self.mustAutoScroll = true
            end
        end
    end
    UI.End()
end

return Terminal
