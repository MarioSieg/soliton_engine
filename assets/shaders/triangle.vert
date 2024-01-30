#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (push_constant) uniform PushConstants {
	mat4 ModelViewProj;
	mat4 NormalMatrix;
} pushConstants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	gl_Position = pushConstants.ModelViewProj * vec4(inPos.xyz, 1.0);
	vec4 nn = pushConstants.NormalMatrix * vec4(inNormal, 0.0);
	outNormal = nn.xyz;
	outUV = inUV;
}
