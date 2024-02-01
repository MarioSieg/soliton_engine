#version 450

layout (set = 1, binding = 0) uniform sampler2D samplerAlbedoMap;
layout (set = 1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec2 outUV;
layout (location = 1) in vec3 outNormal;
layout (location = 2) in vec3 outTangent;
layout (location = 3) in vec3 outBiTangent;
layout (location = 4)  in mat3 outTBN;

layout (location = 0) out vec4 outFragColor;

const float GAMMA = 2.2;
const vec3 VGAMMA = vec3(1.0 / GAMMA);

void main() {
  // Hardcoded light properties
  vec3 lightDir = normalize(vec3(-0.3, -0.8, 0.0)); // Example direction
  vec4 lightColor = vec4(1.0, 0.95, 0.8, 1.0); // Slightly yellowish white

  // Combine texture color with light effect with normal map
  vec4 texColor = texture(samplerAlbedoMap, outUV);

  vec3 normalMap = normalize(texture(samplerNormalMap, outUV).xyz * 2.0 - 1.0);
  vec3 normal = normalize(outTBN * normalMap);

  float diff = max(dot(normal, lightDir), 0.0);

  // add ambient lighting:
  vec4 ambient = vec4(0.1, 0.1, 0.1, 1.0);
  outFragColor = texColor * (ambient + diff * lightColor);

  // gamma correction
  outFragColor.rgb = pow(outFragColor.rgb, VGAMMA);
}