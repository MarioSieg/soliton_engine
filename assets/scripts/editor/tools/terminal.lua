-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'

local ICONS = require 'editor.icons'
local UI = require 'editor.imgui'

local COMMANDS = {}

COMMANDS['panic'] = {
    description = 'Panic!',
    arguments = {},
    execute = function(args)
        app.panic(args[2] or 'Panic!')
    end
}

local Terminal = {
    name = ICONS.LINE_COLUMNS..' Terminal',
    is_visible = ffi.new('bool[1]', true),
    cmdBufLen = 512,
    cmdBuf = ffi.new('char[?]', 512),
    scrollFlags = ffi.new('bool[2]', true, false),
}

function Terminal:render()
    UI.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    local isLuaLogTab = false
    local protocol = protocol
    if UI.Begin(self.name, self.is_visible, ffi.C.ImGuiWindowFlags_NoScrollbar) then
        if UI.BeginTabBar('TerminalTabBar', ffi.C.ImGuiTabBarFlags_None) then
            if UI.BeginTabItem(ICONS.CODE..' Lua') then
                isLuaLogTab = true
                local footer = UI.GetStyle().ItemSpacing.y + UI.GetFrameHeightWithSpacing()
                if UI.BeginChild('TerminalScrollingRegion', UI.ImVec2(0, -footer), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
                    UI.PushStyleVar(ffi.C.ImGuiStyleVar_ItemSpacing, UI.ImVec2(4.0, 1.0))
                    local clipper = UI.ImGuiListClipper()
                    clipper:Begin(#protocol, UI.GetTextLineHeightWithSpacing())
                    while clipper:Step() do -- HOT LOOP
                        for i=clipper.DisplayStart+1, clipper.DisplayEnd do
                            local isError = protocol[i][2]
                            UI.PushStyleColor_U32(ffi.C.ImGuiCol_Text, isError and 0xff4444ff or 0xffffffff)
                            UI.Separator()
                            UI.TextUnformatted(protocol[i][1])
                            UI.PopStyleColor()
                        end
                    end
                    clipper:End()
                    UI.PopStyleVar()
                    if self.scrollFlags[1] then
                        UI.SetScrollHereY(1.0)
                        self.scrollFlags[1] = false
                    end
                    UI.EndChild()
                end
                UI.EndTabItem()
            end
            if UI.BeginTabItem(ICONS.COGS..' System') then
                ffi.C.__lu_dd_draw_native_log(self.scrollFlags[1])
                if self.scrollFlags[1] then
                    self.scrollFlags[1] = false
                end
                UI.EndTabItem()
            end
            UI.Separator()
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
                    self.scrollFlags[1] = true
                    UI.SetKeyboardFocusHere(-1) -- Focus on command line
                end
            end
            UI.SameLine()
            UI.Checkbox(ICONS.MOUSE..' Scroll', self.scrollFlags)
            if self.scrollFlags[0] then
                self.scrollFlags[1] = true
            end
            if isLuaLogTab then
                UI.SameLine()
                if UI.Button(ICONS.TRASH..' Clear') then
                    PROTOCOL = {}
                end
            end
            UI.EndTabBar()
        end
    end
    UI.End()
end

return Terminal
