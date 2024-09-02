// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef PBR_COMMON_H
#define PBR_COMMON_H

#include "shader_common.h"

float distributionGGX(const vec3 N, const vec3 H, const float roughness) {
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH * NdotH;
    const float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = kPI * denom * denom;
    return nom / denom;
}

float geometrySchlickGGX(const float NdotV, const float roughness) {
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;
    const float nom = NdotV;
    const float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float geometrySmith(const vec3 N, const vec3 V, const vec3 L, const float roughness) {
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotL = max(dot(N, L), 0.0);
    const float ggx2 = geometrySchlickGGX(NdotV, roughness);
    const float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(const float cosTheta, const vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(const float cosTheta, const vec3 F0, const float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
vec2 hammersley2D(const uint i, const uint N) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSample_GGX(const vec2 Xi, const float roughness, const vec3 normal) {
    // Maps a 2D point to a hemisphere with spread based on roughness
    const float alpha = roughness * roughness;
    const float phi = 2.0 * kPI * Xi.x + random(normal.xz) * 0.1;
    const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    const float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    const vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    const vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 tangentX = normalize(cross(up, normal));
    const vec3 tangentY = normalize(cross(normal, tangentX));

    // Convert to world Space
    return normalize(tangentX*H.x + tangentY*H.y + normal*H.z);
}

#endif
