local ffi = require 'ffi'
local gui = require 'editor.imgui'

local Style = {}

function Style.setupDarkStyle()
    local style = gui.GetStyle()
    style.WindowPadding     = gui.ImVec2(8.00, 8.00)
    style.FramePadding      = gui.ImVec2(5.00, 2.00)
    style.CellPadding       = gui.ImVec2(6.00, 6.00)
    style.ItemSpacing       = gui.ImVec2(6.00, 6.00)
    style.ItemInnerSpacing  = gui.ImVec2(6.00, 6.00)
    style.TouchExtraPadding = gui.ImVec2(0.00, 0.00)
    style.IndentSpacing     = 25
    style.ScrollbarSize     = 15
    style.GrabMinSize       = 10
    style.WindowBorderSize  = 1
    style.ChildBorderSize   = 1
    style.PopupBorderSize   = 1
    style.FrameBorderSize   = 1
    style.TabBorderSize     = 1
    style.WindowRounding    = 7
    style.ChildRounding     = 4
    style.FrameRounding     = 3
    style.PopupRounding     = 4
    style.ScrollbarRounding = 9
    style.GrabRounding      = 3
    style.LogSliderDeadzone = 4
    style.TabRounding       = 4
    local colors = style.Colors
    colors[ffi.C.ImGuiCol_Text]                     = gui.ImVec4(1.00, 1.00, 1.00, 1.00)
    colors[ffi.C.ImGuiCol_TextDisabled]             = gui.ImVec4(0.50, 0.50, 0.50, 1.00)
    colors[ffi.C.ImGuiCol_WindowBg]                 = gui.ImVec4(0.10, 0.10, 0.10, 1.00)
    colors[ffi.C.ImGuiCol_ChildBg]                  = gui.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_PopupBg]                  = gui.ImVec4(0.19, 0.19, 0.19, 0.92)
    colors[ffi.C.ImGuiCol_Border]                   = gui.ImVec4(0.19, 0.19, 0.19, 0.29)
    colors[ffi.C.ImGuiCol_BorderShadow]             = gui.ImVec4(0.00, 0.00, 0.00, 0.24)
    colors[ffi.C.ImGuiCol_FrameBg]                  = gui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgHovered]           = gui.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgActive]            = gui.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_TitleBg]                  = gui.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgActive]            = gui.ImVec4(0.06, 0.06, 0.06, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgCollapsed]         = gui.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_MenuBarBg]                = gui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarBg]              = gui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrab]            = gui.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabHovered]     = gui.ImVec4(0.40, 0.40, 0.40, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabActive]      = gui.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_CheckMark]                = gui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_SliderGrab]               = gui.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_SliderGrabActive]         = gui.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_Button]                   = gui.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ButtonHovered]            = gui.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_ButtonActive]             = gui.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_Header]                   = gui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_HeaderHovered]            = gui.ImVec4(0.00, 0.00, 0.00, 0.36)
    colors[ffi.C.ImGuiCol_HeaderActive]             = gui.ImVec4(0.20, 0.22, 0.23, 0.33)
    colors[ffi.C.ImGuiCol_Separator]                = gui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorHovered]         = gui.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorActive]          = gui.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_ResizeGrip]               = gui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripHovered]        = gui.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripActive]         = gui.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_Tab]                      = gui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabHovered]               = gui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_TabActive]                = gui.ImVec4(0.20, 0.20, 0.20, 0.36)
    colors[ffi.C.ImGuiCol_TabUnfocused]             = gui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabUnfocusedActive]       = gui.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_DockingPreview]           = gui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_DockingEmptyBg]           = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotLines]                = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotLinesHovered]         = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogram]            = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogramHovered]     = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TableHeaderBg]            = gui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderStrong]        = gui.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderLight]         = gui.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_TableRowBg]               = gui.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_TableRowBgAlt]            = gui.ImVec4(1.00, 1.00, 1.00, 0.06)
    colors[ffi.C.ImGuiCol_TextSelectedBg]           = gui.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_DragDropTarget]           = gui.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_NavHighlight]             = gui.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_NavWindowingHighlight]    = gui.ImVec4(1.00, 0.00, 0.00, 0.70)
    colors[ffi.C.ImGuiCol_NavWindowingDimBg]        = gui.ImVec4(1.00, 0.00, 0.00, 0.20)
    colors[ffi.C.ImGuiCol_ModalWindowDimBg]         = gui.ImVec4(1.00, 0.00, 0.00, 0.35)
    colors[ffi.C.ImGuiCol_PlotLines]                = colors[ffi.C.ImGuiCol_CheckMark]
    colors[ffi.C.ImGuiCol_PlotHistogram]            = colors[ffi.C.ImGuiCol_CheckMark]
end

return Style