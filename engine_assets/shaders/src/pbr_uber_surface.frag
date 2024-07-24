// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

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

layout (push_constant, std430) uniform PushConstants { // TODO: move to per frame cb
  layout(offset = 128) float time;
} pushConstants;

void main() {
  const vec3 tex_color = texture(samplerAlbedoMap, outUV).rgb;
  const vec3 normal = normal_map(outTBN, texture(samplerNormalMap, outUV).xyz);
  vec3 final = diffuse_lambert_lit(tex_color, normal);
  //vec3 final = tex_color;
  final = color_saturation(final, 1.25);
  final = gamma_correct(final);
  final += vec3(film_noise(pushConstants.time*outUV)) * 0.1;
  outFragColor.rgb = final * 0.9;
  outFragColor.a = 1.0;
}