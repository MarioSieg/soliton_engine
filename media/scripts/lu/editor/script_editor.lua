-- Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

local ffi = require 'ffi'
local gui = require 'imgui.gui'

local MAX_SCRIPT_SIZE = 1024 * 512 -- 512 KiB or 1/2 MiB

local M = {
    name = 'Script Editor',
    isVisible = ffi.new('bool[1]', true),
    MAX_SCRIPT_SIZE = MAX_SCRIPT_SIZE,
    textBuf = ffi.new('char[?]', MAX_SCRIPT_SIZE),
}

function M:render()
    gui.SetNextWindowSize(WINDOW_SIZE, ffi.C.ImGuiCond_FirstUseEver)
    if gui.Begin(self.name, self.isVisible) then
       if gui.BeginChild('ScrollingRegion', gui.ImVec2(0, -gui.GetFrameHeightWithSpacing()), false, ffi.C.ImGuiWindowFlags_HorizontalScrollbar) then
            gui.PushID_Int(0xffeefefec0c0)
            if gui.InputTextMultiline('', self.textBuf, MAX_SCRIPT_SIZE, gui.ImVec2(-1, -1), ffi.C.ImGuiInputTextFlags_AllowTabInput) then

            end
            gui.PopID()
            gui.EndChild()
        end
    end
    gui.End()
end

return M