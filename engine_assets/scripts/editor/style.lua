local ffi = require 'ffi'
local ui = require 'imgui.imgui'

local style = {}

function style.setup(scale, round_factor)
    scale = scale or 0.5
    round_factor = round_factor or 2

    local style = ui.GetStyle()
    local colors = style.Colors

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

    colors[ffi.C.ImGuiCol_Text] = ui.ImVec4(1.00, 1.00, 1.00, 1.00)
    colors[ffi.C.ImGuiCol_TextDisabled] = ui.ImVec4(0.50, 0.50, 0.50, 1.00)
    colors[ffi.C.ImGuiCol_WindowBg] = ui.ImVec4(0.10, 0.10, 0.10, 1.00)
    colors[ffi.C.ImGuiCol_ChildBg] = ui.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_PopupBg] = ui.ImVec4(0.19, 0.19, 0.19, 1.00)
    colors[ffi.C.ImGuiCol_Border] = ui.ImVec4(0.28, 0.28, 0.28, 0.80)
    colors[ffi.C.ImGuiCol_BorderShadow] = ui.ImVec4(0.00, 0.00, 0.00, 0.92)
    colors[ffi.C.ImGuiCol_FrameBg] = ui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgHovered] = ui.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgActive] = ui.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_TitleBg] = ui.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgCollapsed] = ui.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgActive] = ui.ImVec4(0.06, 0.06, 0.06, 1.00)
    colors[ffi.C.ImGuiCol_MenuBarBg] = ui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarBg] = ui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrab] = ui.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabHovered] = ui.ImVec4(0.40, 0.40, 0.40, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabActive] = ui.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_CheckMark] = ui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_SliderGrab] = ui.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_SliderGrabActive] = ui.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_Button] = ui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ButtonHovered] = ui.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_ButtonActive] = ui.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_Header] = ui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_HeaderHovered] = ui.ImVec4(0.00, 0.00, 0.00, 0.36)
    colors[ffi.C.ImGuiCol_HeaderActive] = ui.ImVec4(0.20, 0.22, 0.23, 0.33)
    colors[ffi.C.ImGuiCol_Separator] = ui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorHovered] = ui.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorActive] = ui.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_ResizeGrip] = ui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripHovered] = ui.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripActive] = ui.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_Tab] = ui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabHovered] = ui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_TabSelected] = ui.ImVec4(0.20, 0.20, 0.20, 0.36)
    colors[ffi.C.ImGuiCol_TabSelectedOverline] = ui.ImVec4(0.20, 0.20, 0.20, 0.36)
    colors[ffi.C.ImGuiCol_TabDimmed] = ui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabDimmedSelected] = ui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_TabDimmedSelectedOverline] = ui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_DockingPreview] = ui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_DockingEmptyBg] = ui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotLines] = ui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_PlotLinesHovered] = ui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogram] = ui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogramHovered] = ui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TableHeaderBg] = ui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderStrong] = ui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderLight] = ui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_TableRowBg] = ui.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_TableRowBgAlt] = ui.ImVec4(0.06, 1.00, 1.00, 1.00)
    colors[ffi.C.ImGuiCol_TextSelectedBg] = ui.ImVec4(1.00, 0.20, 0.22, 0.23)
    colors[ffi.C.ImGuiCol_DragDropTarget] = ui.ImVec4(0.33, 0.67, 0.86, 1.00)
    --colors[ffi.C.ImGuiCol_NavHighlight] = ui.ImVec4(1.00, 0.00, 0.00, 1.00)
    --colors[ffi.C.ImGuiCol_NavWindowingHighlight] = ui.ImVec4(0.70, 1.00, 0.00, 0.00)
    --colors[ffi.C.ImGuiCol_NavWindowingDimBg] = ui.ImVec4(0.20, 1.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_ModalWindowDimBg] = ui.ImVec4(0.80, 0.35, 0.35, 0.35)
end

return style
