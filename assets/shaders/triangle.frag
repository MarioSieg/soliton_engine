#version 450

layout (set = 1, binding = 0) uniform sampler2D samplerAlbedoMap;
layout (set = 1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec2 outUV;
layout (location = 1) in vec3 outNormal;
layout (location = 2) in vec3 outTangent;
layout (location = 3) in vec3 outBiTangent;
layout (location = 4) in mat3 outTBN;

layout (location = 0) out vec4 outFragColor;

const float GAMMA = 2.2;
const vec3 VGAMMA = vec3(1.0 / GAMMA);

// Hardcoded light properties
const vec3 lightDir = vec3(0.0, -1.0, -0.3); // Example direction
const vec4 lightColor = vec4(1.0, 0.95, 0.8, 1.0); // Slightly yellowish white
// add ambient lighting:
const vec4 ambient = vec4(0.1, 0.1, 0.15, 1.0);
const float normalMapStrength = 2.0;

void main() {
  vec4 texColor = texture(samplerAlbedoMap, outUV);
  vec3 normalMap = normalize((texture(samplerNormalMap, outUV).xyz * 2.0 - 1.0) * normalMapStrength);
  vec3 normal = normalize(outTBN * normalMap);
  float diff = max(dot(normal, lightDir), 0.0);
  outFragColor = texColor; // * (ambient + diff * lightColor);

  // gamma correction
  outFragColor.rgb = pow(outFragColor.rgb, VGAMMA);
}