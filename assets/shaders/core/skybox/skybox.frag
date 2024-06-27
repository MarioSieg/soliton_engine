// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include <lunam_shader_common.glsli>

layout (set = 0, binding = 0) uniform sampler2D samplerAlbedoMap;

layout (location = 0) in vec2 outUV;

layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = vec4(1);
}