#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec3 outNormal;
layout (location = 1) in vec2 outUV;
layout (location = 0) out vec4 outFragColor;

void main() {
  // Hardcoded light properties
  vec3 lightDir = normalize(vec3(0.0, 1.0, 0.0)); // Example direction
  vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0); // White light

  // Calculate diffuse lighting
  float diff = max(dot(normalize(outNormal), lightDir), 0.0);

  // Combine texture color with light effect
  vec4 texColor = texture(samplerColor, outUV);
  outFragColor = texColor * lightColor * diff;
}