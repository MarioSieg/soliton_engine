// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (push_constant, std430) uniform PushConstants {
	mat4 ModelViewProj;
	mat4 NormalMatrix;
} pushConstants;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outTangent;
layout (location = 3) out vec3 outBiTangent;
layout (location = 4) out mat3 outTBN;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position = pushConstants.ModelViewProj * vec4(inPos.xyz, 1.0);
	mat3 nn = mat3(pushConstants.NormalMatrix);
	outNormal = nn * inNormal;
	outTangent = nn * inTangent;
	outBiTangent = nn * inBiTangent;
	outTBN = mat3(normalize(outTangent), normalize(outBiTangent), normalize(outNormal));
	outUV = inUV;
}
