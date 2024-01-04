// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

namespace ImGuiEx {
    extern auto Create(GLFWwindow* window, float font_size) -> void;
    extern auto Destroy() -> void;
    extern auto BeginFrame(uint16_t _width, uint16_t _height, bgfx::ViewId _viewId) -> void;
    extern auto EndFrame() -> void;
}
