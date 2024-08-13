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
     layout(offset = 192) vec4 camera_pos; // xyz: camera position, w: time
     //layout(offset = 144) vec4 light_dir;  // xyz: normalized light direction, w: unused
     //layout(offset = 160) vec4 light_color; // xyz: light color, w: intensity
} consts;

const float MAX_REFLECTION_LOD = 4.0;

void main() {
  const vec3 albedo = texture(samplerAlbedoMap, inUV).rgb;
  const vec2 metallic_roughness = texture(samplerRoughness, inUV).rg;
  const float metallic = metallic_roughness.r;
  const float roughness = metallic_roughness.g;
  const float ao = 1.0; //texture(samplerAO, inUV).r;

  const vec3 N = normal_map(inTBN, texture(samplerNormalMap, inUV).xyz);
  const vec3 V = normalize(consts.camera_pos.xyz - inWorldPos);
  const vec3 R = reflect(-V, N);

  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.0);
  {
    // calculate per-light radiance
    vec3 L = normalize(CB_PER_FRAME.sun_dir);
    vec3 H = normalize(V + L);
    vec3 radiance = CB_PER_FRAME.sun_color * 4;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // add to outgoing radiance Lo
    Lo += (kD * albedo / kPI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  }

  // ambient lighting (we now use IBL as the ambient term)
  vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = texture(irradiance_cube, -N).rgb;
  vec3 diffuse = irradiance * albedo;

  // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
  vec3 prefilteredColor = textureLod(prefilter_cube, -R, roughness * MAX_REFLECTION_LOD).rgb;
  vec2 brdf  = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

  vec3 ambient = (kD * diffuse + specular) * ao;

  vec3 color = ambient + Lo;

  outFragColor.rgb = color;
  outFragColor.a = 1.0;
}