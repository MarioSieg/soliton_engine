// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"
#include "cpp_shared_structures.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBiTangent;
layout (location = 5) out vec3 outTangentViewPos;
layout (location = 6) out vec3 outTangentFragPos;
layout (location = 7) out mat3 outTBN;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (std140, push_constant, std430) uniform PushConstants {
	mat4 ModelMatrix;
	mat4 ModelViewProj;
	mat4 NormalMatrix;
} consts;

void main() {
	outWorldPos = vec3(consts.ModelMatrix * vec4(inPos, 1.0));
	mat3 nn = mat3(consts.ModelMatrix);
	outNormal = nn * normalize(inNormal);
	outTangent = nn * normalize(inTangent);
	outBiTangent = nn * normalize(inBiTangent);
	outTBN = mat3(outTangent, outBiTangent, outNormal);
	outTangentViewPos = outTBN * uboPerFrame.camPos.xyz;
	outTangentFragPos = outTBN * outWorldPos;
	outUV = inUV;
	gl_Position = consts.ModelViewProj * vec4(inPos.xyz, 1.0);
}
