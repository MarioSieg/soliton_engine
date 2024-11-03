// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outUVW;

layout (push_constant) uniform PushConstants {
	mat4 view;
	mat4 proj;
} push_consts;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	outUVW = inPos;
	outUVW.xy *= -1.0; // Convert cubemap coordinates into Vulkan coordinate space
	const mat4 viewMat = mat4(mat3(push_consts.view)); // Remove translation from view matrix
	gl_Position = (push_consts.proj * viewMat * vec4(inPos.xyz, 1.0)).xyww;
}
