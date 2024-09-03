// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "pbr_common.h"
#include "filmic_tonemapper.h"
#include "cpp_shared_structures.h"

layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME, binding = 0) uniform uniformPerFrameUBO {
  perFrameData uboPerFrame;
};

layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 0) uniform sampler2D sAlbedoMap;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 1) uniform sampler2D sNormalMap;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 2) uniform sampler2D sRoughnessMap;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 3) uniform sampler2D sHeightMap;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 4) uniform sampler2D sOcclusionMap;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL, binding = 5) uniform sampler2D sEmissionMap;

layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM, binding = 0) uniform sampler2D brdf_lut;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM, binding = 1) uniform samplerCube irradiance_cube;
layout (set = LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM, binding = 2) uniform samplerCube prefilter_cube;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in vec3 inTangentViewPos;
layout (location = 6) in vec3 inTangentFragPos;
layout (location = 7) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

const float MAX_REFLECTION_LOD = 9.0;

vec3 calculateNormal(vec2 uv)
{
  vec3 tangentNormal = texture(sNormalMap, uv).xyz * 2.0 - 1.0;
  vec3 N = normalize(inNormal);
  vec3 T = normalize(inTangent.xyz);
  vec3 B = normalize(cross(N, T));
  mat3 TBN = mat3(T, B, N);
  return normalize(TBN * tangentNormal);
}

void main() {
  const vec2 uv = inUV;
  const vec3 albedo = texture(sAlbedoMap, uv).rgb;
  const vec2 metallic_roughness = texture(sRoughnessMap, uv).rg;
  const float metallic = metallic_roughness.r;
  const float roughness = metallic_roughness.g;
  const float ao = texture(sOcclusionMap, inUV).r;
  const vec3 emissive = texture(sEmissionMap, inUV).rgb;

  const vec3 V = normalize(inWorldPos - uboPerFrame.camPos.xyz);
  const vec3 N = normalMap(inTBN, texture(sNormalMap, uv).rgb);
  const vec3 R = reflect(-V, N);

  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.0);
  {
    // calculate per-light radiance
    vec3 L = normalize(uboPerFrame.sunDir.xyz);
    vec3 H = normalize(V + L);
    vec3 radiance = uboPerFrame.sunColor.rgb;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
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
    float NdotL = clamp(dot(N, L), 0.0, 1.0);

    // add to outgoing radiance Lo
    Lo = (kD * albedo / kPI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  }

  // ambient lighting (we now use IBL as the ambient term)
  vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = texture(irradiance_cube, N).rgb;
  vec3 diffuse = irradiance * albedo;

  // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
  vec3 prefilteredColor = textureLod(prefilter_cube, R, roughness * MAX_REFLECTION_LOD).rgb;
  vec2 brdf = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

  vec3 ambient = (kD * diffuse + specular) * ao;

  vec3 color = ambient + Lo + emissive;

  // Tone mapping
  color = postToneMap(color);

  outFragColor.rgb = color;
  outFragColor.a = 1.0;
}