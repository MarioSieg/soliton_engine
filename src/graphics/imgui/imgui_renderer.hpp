// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

extern auto imguiCreate(GLFWwindow* window) -> void;
extern auto imguiDestroy() -> void;
extern auto imguiBeginFrame(uint16_t _width, uint16_t _height, bgfx::ViewId _viewId) -> void;
extern auto imguiEndFrame() -> void;
