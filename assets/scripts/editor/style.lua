local ffi = require 'ffi'
local UI = require 'editor.imgui'

local Style = {}

function Style.setupDarkStyle()
    local style = UI.GetStyle()
    local colors = style.Colors

    style.WindowPadding     = UI.ImVec2(8.00, 8.00)
    style.FramePadding      = UI.ImVec2(5.00, 2.00)
    style.CellPadding       = UI.ImVec2(6.00, 6.00)
    style.ItemSpacing       = UI.ImVec2(6.00, 6.00)
    style.ItemInnerSpacing  = UI.ImVec2(6.00, 6.00)
    style.TouchExtraPadding = UI.ImVec2(0.00, 0.00)
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
    colors[ffi.C.ImGuiCol_Text]                     = UI.ImVec4(1.00, 1.00, 1.00, 1.00)
    colors[ffi.C.ImGuiCol_TextDisabled]             = UI.ImVec4(0.50, 0.50, 0.50, 1.00)
    colors[ffi.C.ImGuiCol_WindowBg]                 = UI.ImVec4(0.10, 0.10, 0.10, 1.00)
    colors[ffi.C.ImGuiCol_ChildBg]                  = UI.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_PopupBg]                  = UI.ImVec4(0.19, 0.19, 0.19, 0.92)
    colors[ffi.C.ImGuiCol_Border]                   = UI.ImVec4(0.28, 0.28, 0.28, 0.8)
	colors[ffi.C.ImGuiCol_BorderShadow]             = UI.ImVec4(0.92, 0.91, 0.88, 0.0)
    colors[ffi.C.ImGuiCol_FrameBg]                  = UI.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgHovered]           = UI.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_FrameBgActive]            = UI.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_TitleBg]                  = UI.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgActive]            = UI.ImVec4(0.06, 0.06, 0.06, 1.00)
    colors[ffi.C.ImGuiCol_TitleBgCollapsed]         = UI.ImVec4(0.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_MenuBarBg]                = UI.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_ScrollbarBg]              = UI.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrab]            = UI.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabHovered]     = UI.ImVec4(0.40, 0.40, 0.40, 0.54)
    colors[ffi.C.ImGuiCol_ScrollbarGrabActive]      = UI.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_CheckMark]                = UI.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_SliderGrab]               = UI.ImVec4(0.34, 0.34, 0.34, 0.54)
    colors[ffi.C.ImGuiCol_SliderGrabActive]         = UI.ImVec4(0.56, 0.56, 0.56, 0.54)
    colors[ffi.C.ImGuiCol_Button]                   = UI.ImVec4(0.05, 0.05, 0.05, 0.54)
    colors[ffi.C.ImGuiCol_ButtonHovered]            = UI.ImVec4(0.19, 0.19, 0.19, 0.54)
    colors[ffi.C.ImGuiCol_ButtonActive]             = UI.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_Header]                   = UI.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_HeaderHovered]            = UI.ImVec4(0.00, 0.00, 0.00, 0.36)
    colors[ffi.C.ImGuiCol_HeaderActive]             = UI.ImVec4(0.20, 0.22, 0.23, 0.33)
    colors[ffi.C.ImGuiCol_Separator]                = UI.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorHovered]         = UI.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_SeparatorActive]          = UI.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_ResizeGrip]               = UI.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripHovered]        = UI.ImVec4(0.44, 0.44, 0.44, 0.29)
    colors[ffi.C.ImGuiCol_ResizeGripActive]         = UI.ImVec4(0.40, 0.44, 0.47, 1.00)
    colors[ffi.C.ImGuiCol_Tab]                      = UI.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabHovered]               = UI.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_TabActive]                = UI.ImVec4(0.20, 0.20, 0.20, 0.36)
    colors[ffi.C.ImGuiCol_TabUnfocused]             = UI.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TabUnfocusedActive]       = UI.ImVec4(0.14, 0.14, 0.14, 1.00)
    colors[ffi.C.ImGuiCol_DockingPreview]           = UI.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_DockingEmptyBg]           = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotLines]                = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotLinesHovered]         = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogram]            = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_PlotHistogramHovered]     = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_TableHeaderBg]            = UI.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderStrong]        = UI.ImVec4(0.00, 0.00, 0.00, 0.52)
    colors[ffi.C.ImGuiCol_TableBorderLight]         = UI.ImVec4(0.28, 0.28, 0.28, 0.29)
    colors[ffi.C.ImGuiCol_TableRowBg]               = UI.ImVec4(0.00, 0.00, 0.00, 0.00)
    colors[ffi.C.ImGuiCol_TableRowBgAlt]            = UI.ImVec4(1.00, 1.00, 1.00, 0.06)
    colors[ffi.C.ImGuiCol_TextSelectedBg]           = UI.ImVec4(0.20, 0.22, 0.23, 1.00)
    colors[ffi.C.ImGuiCol_DragDropTarget]           = UI.ImVec4(0.33, 0.67, 0.86, 1.00)
    colors[ffi.C.ImGuiCol_NavHighlight]             = UI.ImVec4(1.00, 0.00, 0.00, 1.00)
    colors[ffi.C.ImGuiCol_NavWindowingHighlight]    = UI.ImVec4(1.00, 0.00, 0.00, 0.70)
    colors[ffi.C.ImGuiCol_NavWindowingDimBg]        = UI.ImVec4(1.00, 0.00, 0.00, 0.20)
    colors[ffi.C.ImGuiCol_ModalWindowDimBg]         = UI.ImVec4(1.00, 0.00, 0.00, 0.35)
    colors[ffi.C.ImGuiCol_PlotLines]                = colors[ffi.C.ImGuiCol_CheckMark]
    colors[ffi.C.ImGuiCol_PlotHistogram]            = colors[ffi.C.ImGuiCol_CheckMark]
end

function Style.setupDarkStyleNoRounding()
    Style.setupDarkStyle()
    local style = UI.GetStyle()
    style.WindowRounding    = 0
    style.ChildRounding     = 0
    style.FrameRounding     = 0
    style.PopupRounding     = 0
    style.ScrollbarRounding = 0
    style.GrabRounding      = 0
    style.TabRounding       = 0
end

function Style.setupDarkStyleModern()
    local style = UI.GetStyle()
    local colors = style.Colors

    style.WindowPadding = UI.ImVec2(15, 15)
	style.WindowRounding = 5.0
	style.FramePadding = UI.ImVec2(5, 5)
	style.FrameRounding = 4.0
	style.ItemSpacing = UI.ImVec2(12, 8)
	style.ItemInnerSpacing = UI.ImVec2(8, 6)
	style.IndentSpacing = 25.0
	style.ScrollbarSize = 15.0
	style.ScrollbarRounding = 9.0
	style.GrabMinSize = 5.0
	style.GrabRounding = 3.0

	colors[ffi.C.ImGuiCol_Text]                         = UI.ImVec4(0.80, 0.80, 0.83, 1.00)
	colors[ffi.C.ImGuiCol_TextDisabled]                 = UI.ImVec4(0.24, 0.23, 0.29, 1.00)
	colors[ffi.C.ImGuiCol_WindowBg]                     = UI.ImVec4(0.06, 0.05, 0.07, 1.00)
	colors[ffi.C.ImGuiCol_PopupBg]                      = UI.ImVec4(0.07, 0.07, 0.09, 1.00)
	colors[ffi.C.ImGuiCol_Border]                       = UI.ImVec4(0.80, 0.80, 0.83, 0.88)
	colors[ffi.C.ImGuiCol_BorderShadow]                 = UI.ImVec4(0.92, 0.91, 0.88, 0.00)
	colors[ffi.C.ImGuiCol_FrameBg]                      = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_FrameBgHovered]               = UI.ImVec4(0.24, 0.23, 0.29, 1.00)
	colors[ffi.C.ImGuiCol_FrameBgActive]                = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_TitleBg]                      = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_TitleBgCollapsed]             = UI.ImVec4(1.00, 0.98, 0.95, 0.75)
	colors[ffi.C.ImGuiCol_TitleBgActive]                = UI.ImVec4(0.07, 0.07, 0.09, 1.00)
	colors[ffi.C.ImGuiCol_MenuBarBg]                    = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_ScrollbarBg]                  = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_ScrollbarGrab]                = UI.ImVec4(0.80, 0.80, 0.83, 0.31)
	colors[ffi.C.ImGuiCol_ScrollbarGrabHovered]         = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_ScrollbarGrabActive]          = UI.ImVec4(0.06, 0.05, 0.07, 1.00)
	colors[ffi.C.ImGuiCol_CheckMark]                    = UI.ImVec4(0.80, 0.80, 0.83, 0.31)
	colors[ffi.C.ImGuiCol_SliderGrab]                   = UI.ImVec4(0.80, 0.80, 0.83, 0.31)
	colors[ffi.C.ImGuiCol_SliderGrabActive]             = UI.ImVec4(0.06, 0.05, 0.07, 1.00)
	colors[ffi.C.ImGuiCol_Button]                       = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_ButtonHovered]                = UI.ImVec4(0.24, 0.23, 0.29, 1.00)
	colors[ffi.C.ImGuiCol_ButtonActive]                 = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_Header]                       = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_HeaderHovered]                = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_HeaderActive]                 = UI.ImVec4(0.06, 0.05, 0.07, 1.00)
	colors[ffi.C.ImGuiCol_Tab]                          = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_TabHovered]                   = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_TabActive]                    = UI.ImVec4(0.24, 0.23, 0.29, 1.00)
    colors[ffi.C.ImGuiCol_TabUnfocused]                 = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
    colors[ffi.C.ImGuiCol_TabUnfocusedActive]           = UI.ImVec4(0.10, 0.09, 0.12, 1.00)
	colors[ffi.C.ImGuiCol_ResizeGrip]                   = UI.ImVec4(0.00, 0.00, 0.00, 0.00)
	colors[ffi.C.ImGuiCol_ResizeGripHovered]            = UI.ImVec4(0.56, 0.56, 0.58, 1.00)
	colors[ffi.C.ImGuiCol_ResizeGripActive]             = UI.ImVec4(0.06, 0.05, 0.07, 1.00)
	colors[ffi.C.ImGuiCol_PlotLines]                    = UI.ImVec4(0.40, 0.39, 0.38, 0.63)
	colors[ffi.C.ImGuiCol_PlotLinesHovered]             = UI.ImVec4(0.25, 1.00, 0.00, 1.00)
	colors[ffi.C.ImGuiCol_PlotHistogram]                = UI.ImVec4(0.40, 0.39, 0.38, 0.63)
	colors[ffi.C.ImGuiCol_PlotHistogramHovered]         = UI.ImVec4(0.25, 1.00, 0.00, 1.00)
	colors[ffi.C.ImGuiCol_TextSelectedBg]               = UI.ImVec4(0.25, 1.00, 0.00, 0.43)
	--colors[ffi.C.ImGuiCol_ModalWindowDarkening]         = UI.ImVec4(1.00, 0.98, 0.95, 0.73)
end

return Style
