// Copyright (c) 2022-2023 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include <lunam_shader_common.glsli>

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (location = 0) out vec2 outUV;

layout (push_constant) uniform PushConstants {
	mat4 ModelViewProj;
} pushConstants;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = pushConstants.ModelViewProj * vec4(inPos.xyz, 1.0);
	outUV = inUV;
}
