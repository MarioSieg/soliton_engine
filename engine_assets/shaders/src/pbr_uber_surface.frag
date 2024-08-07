// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

layout (set = 0, binding = 0) uniform sampler2D samplerAlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D samplerNormalMap;
layout (set = 0, binding = 2) uniform sampler2D samplerRoughness;
layout (set = 0, binding = 3) uniform sampler2D samplerAO;

layout (set = 1, binding = 0) uniform sampler2D brdf_lut;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

layout (push_constant, std430) uniform PushConstants { // TODO: move to per frame cb
  layout(offset = 128) vec4 camera_pos; // xyz: camera position, w: time
} consts;

vec3 prefiltered_reflection(const vec3 R, const float roughness) {
  const float MAX_REFLECTION_LOD = 9.0;
  float lod = roughness * MAX_REFLECTION_LOD;
  float lodf = floor(lod);
  float lodc = ceil(lod);
  vec3 a = vec3(1.0);
  vec3 b = vec3(1.0);
  return mix(a, b, lod - lodf);
}

void main() {
  const vec3 N = normal_map(inTBN, texture(samplerNormalMap, inUV).xyz);
  const vec3 V = normalize(consts.camera_pos.xyz - inWorldPos);
  const vec3 R = reflect(-V, N);
  const vec2 metallicRoughness = texture(samplerRoughness, inUV).rg;
  const float metallic = metallicRoughness.r;
  const float roughness = metallicRoughness.g;
  const vec4 albedo = texture(samplerAlbedoMap, inUV);
  vec3 F0 = mix(vec3(0.04), pow(albedo.rgb, VGAMMA), metallic);
  vec3 Lo = vec3(0.0);
  vec3 L = normalize(vec3(0.0) - inWorldPos);
  Lo += pbr_specular_contrib(albedo.rgb, L, V, N, F0, metallic, roughness);
  outFragColor.rgb = albedo.rgb;
  outFragColor.a = albedo.a;
}