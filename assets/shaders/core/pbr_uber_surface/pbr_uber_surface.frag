// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include <lunam_shader_common.glsli>

layout (set = 0, binding = 0) uniform sampler2D samplerAlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D samplerNormalMap;
layout (set = 0, binding = 2) uniform sampler2D samplerRoughness;
layout (set = 0, binding = 3) uniform sampler2D samplerAO;

layout (location = 0) in vec2 outUV;
layout (location = 1) in vec3 outNormal;
layout (location = 2) in vec3 outTangent;
layout (location = 3) in vec3 outBiTangent;
layout (location = 4) in mat3 outTBN;

layout (location = 0) out vec4 outFragColor;

void main() {
  const vec3 tex_color = texture(samplerAlbedoMap, outUV).rgb;
  const vec3 normal = normal_map(outTBN, texture(samplerNormalMap, outUV).xyz);
  outFragColor.rgb = diffuse_lambert_lit(tex_color, normal);

  // gamma correction
  outFragColor.rgb = gamma_correct(outFragColor.rgb);
  outFragColor.a = 1.0;
}