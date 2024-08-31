// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

layout (set = 0, binding = 0) uniform samplerCube samplerSkybox;

layout (location = 0) in vec3 inUVW;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(samplerSkybox, inUVW);
}