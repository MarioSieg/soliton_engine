#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (binding = 0) uniform UBO {
	mat4 mvp;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	outColor = inNormal;
	gl_Position = ubo.mvp * vec4(inPos.xyz, 1.0);
}
