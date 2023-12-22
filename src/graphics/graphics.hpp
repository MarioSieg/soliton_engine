// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#pragma once

#include "../core/core.hpp"

#include <GLFW/glfw3.h>

namespace Graphics {
    extern auto InitGraphics(GLFWwindow* window, void* nativeWindow) -> void;
    extern auto BeginFrame() -> void;
    extern auto EndFrame() -> void;
    extern auto OnResize() -> void;
    extern auto ShutdownGraphics() -> void;
}
