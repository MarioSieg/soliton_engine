// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"

layout (set = 0, binding = 0) uniform sampler2D samplerAlbedoMap;
layout (set = 0, binding = 1) uniform sampler2D samplerNormalMap;
layout (set = 0, binding = 2) uniform sampler2D samplerRoughness;
layout (set = 0, binding = 3) uniform sampler2D samplerAO;

layout (set = 1, binding = 0) uniform sampler2D brdf_lut;
layout (set = 1, binding = 1) uniform samplerCube irradiance_cube;
layout (set = 1, binding = 2) uniform samplerCube prefilter_cube;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

layout (push_constant, std430) uniform PushConstants { // TODO: move to per frame cb
     layout(offset = 128) vec4 camera_pos; // xyz: camera position, w: time
     //layout(offset = 144) vec4 light_dir;  // xyz: normalized light direction, w: unused
     //layout(offset = 160) vec4 light_color; // xyz: light color, w: intensity
} consts;

vec3 prefiltered_reflection(const vec3 R, const float roughness) {
  const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
  float lod = roughness * MAX_REFLECTION_LOD;
  float lodf = floor(lod);
  float lodc = ceil(lod);
  vec3 a = textureLod(prefilter_cube, R, lodf).rgb;
  vec3 b = textureLod(prefilter_cube, R, lodc).rgb;
  return mix(a, b, lod - lodf);
}

void main() {
  const vec3 N = normal_map(inTBN, texture(samplerNormalMap, inUV).xyz);
  const vec3 V = normalize(consts.camera_pos.xyz - inWorldPos);
  const vec3 R = reflect(-V, N);
  const vec2 metallic_roughness = texture(samplerRoughness, inUV).rg;
  const float metallic = metallic_roughness.r;
  const float roughness = metallic_roughness.g;
  const vec4 albedo = texture(samplerAlbedoMap, inUV);

  vec3 F0 = mix(vec3(0.04), pow(albedo.rgb, VGAMMA), metallic);
  vec3 Lo = vec3(0.0);

  // Use light direction from push constants
  vec3 L = normalize(-CB_PER_FRAME.sun_dir); // Normalize in case it's not

  // Calculate PBR specular contribution for the directional light
  Lo += pbr_specular_contrib(albedo.rgb, L, V, N, F0, metallic, roughness); //* consts.light_color.a;

  vec2 brdf = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec3 reflection = prefiltered_reflection(R, roughness).rgb;
  vec3 irradiance = texture(irradiance_cube, N).rgb;

  // Diffuse based on irradiance
  vec3 diffuse = irradiance * albedo.rgb;

  vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);

  // Specular reflectance
  vec3 specular = reflection * (F * brdf.x + brdf.y);

  // Ambient part
  vec3 kD = 1.0 - F;
  kD *= 1.0 - metallic;
  vec3 ambient = (kD * diffuse + specular); //* texture(aoMap, inUV).rrr;

  vec3 color = ambient + Lo;

  outFragColor.rgb = color;
  outFragColor.a = albedo.a;
}