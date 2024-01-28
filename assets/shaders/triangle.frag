#version 450

layout (location = 0) in vec3 inNormalColor;
layout (location = 0) out vec4 outFragColor;

void main() {
  outFragColor = vec4(0.5 * inNormalColor + 0.5, 1.0);
}