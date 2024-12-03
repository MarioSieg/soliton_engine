local ffi = require 'ffi'
local ui = require 'imgui.imgui'

local style = {}

function style.setup()
    local style = ui.GetStyle()
    local colors = style.Colors

    local scale = 0.5
    local round_factor = 2

    style.WindowPadding     = ui.ImVec2(15*scale, 15*scale)
    style.FramePadding      = ui.ImVec2(9*scale, 9*scale)
    style.CellPadding       = ui.ImVec2(7*scale, 7*scale)
    style.ItemSpacing       = ui.ImVec2(12*scale, 8*scale)
    style.ItemInnerSpacing  = ui.ImVec2(8*scale, 6*scale)
    style.TouchExtraPadding = ui.ImVec2(0*scale, 0*scale)
    style.IndentSpacing     = 25
    style.ScrollbarSize     = 15
    style.GrabMinSize       = 10
    style.WindowBorderSize  = 1
    style.ChildBorderSize   = 1
    style.PopupBorderSize   = 1
    style.FrameBorderSize   = 1
    style.TabBorderSize     = 1
    style.WindowRounding    = 7*round_factor
    style.ChildRounding     = 4*round_factor
    style.FrameRounding     = 3*round_factor
    style.PopupRounding     = 4*round_factor
    style.ScrollbarRounding = 9*round_factor
    style.GrabRounding      = 3*round_factor
    style.TabRounding       = 4*round_factor
    style.LogSliderDeadzone = 4

    colors[ffi.C.ImGuiCol_Text] = ui.ImVec4(0.80, 0.80, 0.83, 1.00)
    colors[ffi.C.ImGuiCol_TextDisabled] = ui.ImVec4(0.24, 0.23, 0.29, 1.00)
    colors[ffi.C.ImGuiCol_WindowBg] = ui.ImVec4(0.06, 0.05, 0.07, 1.00)
    colors[ffi.C.ImGuiCol_PopupBg] = ui.ImVec4(0.07, 0.07, 0.09, 1.00)
    colors[ffi.C.ImGuiCol_Border] = ui.ImVec4(0.80, 0.80, 0.83, 0.88)
    colors[ffi.C.ImGuiCol_BorderShadow] = ui.ImVec4(0.92, 0.91, 0.88, 0.00)
    colors[ffi.C.ImGuiCol_FrameBg] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_FrameBgHovered] = ui.ImVec4(0.24, 0.23, 0.29, 1.00)
    colors[ffi.C.ImGuiCol_FrameBgActive] = ui.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_TitleBg] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgCollapsed] = ui.ImVec4(1.00, 0.98, 0.95, 0.75)
    colors[ffi.C.ImGuiCol_TitleBgActive] = ui.ImVec4(0.07, 0.07, 0.09, 1.00)
    colors[ffi.C.ImGuiCol_MenuBarBg] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarBg] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarGrab] = ui.ImVec4(0.80, 0.80, 0.83, 0.31)
    colors[ffi.C.ImGuiCol_ScrollbarGrabHovered] = ui.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarGrabActive] = ui.ImVec4(0.06, 0.05, 0.07, 1.00)
    colors[ffi.C.ImGuiCol_CheckMark] = ui.ImVec4(0.80, 0.80, 0.83, 0.31)
    colors[ffi.C.ImGuiCol_SliderGrab] = ui.ImVec4(0.80, 0.80, 0.83, 0.31)
    colors[ffi.C.ImGuiCol_SliderGrabActive] = ui.ImVec4(0.06, 0.05, 0.07, 1.00)
    colors[ffi.C.ImGuiCol_Button] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_ButtonHovered] = ui.ImVec4(0.24, 0.23, 0.29, 1.00)
    colors[ffi.C.ImGuiCol_ButtonActive] = ui.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_Header] = ui.ImVec4(0.10, 0.09, 0.12, 1.00)
    colors[ffi.C.ImGuiCol_HeaderHovered] = ui.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_HeaderActive] = ui.ImVec4(0.06, 0.05, 0.07, 1.00)
    colors[ffi.C.ImGuiCol_ResizeGrip] = ui.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_ResizeGripHovered] = ui.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_ResizeGripActive] = ui.ImVec4(0.06, 0.05, 0.07, 1.00)
    colors[ffi.C.ImGuiCol_PlotLines] = ui.ImVec4(0.40, 0.39, 0.38, 0.63)
    colors[ffi.C.ImGuiCol_PlotLinesHovered] = ui.ImVec4(0.25, 1.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogram] = ui.ImVec4(0.40, 0.39, 0.38, 0.63)
    colors[ffi.C.ImGuiCol_PlotHistogramHovered] = ui.ImVec4(0.25, 1.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TextSelectedBg] = ui.ImVec4(0.25, 1.00, 0.00, 0.43)
    colors[ffi.C.ImGuiCol_Tab] = colors[ffi.C.ImGuiCol_Button]
    colors[ffi.C.ImGuiCol_TabSelected] = colors[ffi.C.ImGuiCol_ButtonHovered]
    colors[ffi.C.ImGuiCol_TabHovered] = colors[ffi.C.ImGuiCol_ButtonActive]
    colors[ffi.C.ImGuiCol_TabDimmed] = colors[ffi.C.ImGuiCol_Button]
    colors[ffi.C.ImGuiCol_TabDimmedSelected] = colors[ffi.C.ImGuiCol_Button]
end

return style
