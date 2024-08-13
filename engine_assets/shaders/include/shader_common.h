// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

const float kPI = 3.1415926535897932384626433832795;

const float GAMMA = 2.2; // sRGB gamma todo: make variable
const vec3 VGAMMA = vec3(1.0 / GAMMA);

// Sun and scene lighting properties. Updated once per frame.
struct cb_per_frame {
    vec3 sun_dir;
    vec3 sun_color;
    vec3 const_ambient;
};

// PBR per-material properties. Updated once per material.
struct cb_per_material {
    vec3 albedo;
    float normal_scale;
    float metallic;
    float roughness;
};

const cb_per_frame CB_PER_FRAME = cb_per_frame(
    vec3(0.0, -1.0, 0.0),
    vec3(1.0, 1.0, 1.0),
    vec3(0.1, 0.1, 0.2)
);

const cb_per_material CB_PER_MAT = cb_per_material(
    vec3(0.5, 0.5, 0.9),
    1.0,
    0.0,
    0.5
);

vec3 diffuse_lambert_lit(const vec3 color, const vec3 n) {
    const float diff = max(dot(n, CB_PER_FRAME.sun_dir), 0.0);
    return color * (CB_PER_FRAME.const_ambient + diff * CB_PER_FRAME.sun_color);
}

vec3 gamma_correct(const vec3 color) {
    return pow(color, VGAMMA);
}

vec3 normal_map(const mat3 tbn, const vec3 n) {
    const vec3 n_map = normalize((n * 2.0 - 1.0) * CB_PER_MAT.normal_scale);
    return normalize(tbn * n_map);
}

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 uncharted2_tonemap(const vec3 color) {
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	const float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 color_saturation(const vec3 rgb, const float adjustment) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    const vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

float film_noise(const vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co) {
    const float a = 12.9898;
    const float b = 78.233;
    const float c = 43758.5453;
    const float dt= dot(co.xy ,vec2(a,b));
    const float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importance_sample_ggx(const vec2 Xi, const float roughness, const vec3 normal) {
    // Maps a 2D point to a hemisphere with spread based on roughness
    const float alpha = roughness * roughness;
    const float phi = 2.0 * kPI * Xi.x + random(normal.xz) * 0.1;
    const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    const vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    const vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 tangentX = normalize(cross(up, normal));
    const vec3 tangentY = normalize(cross(normal, tangentX));

    // Convert to world Space
    return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float d_ggx(const float dotNH, const float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(kPI * denom*denom);
}


// Geometric Shadowing function
float g_geometry_schlick_ggx(const float dotNL, const float dotNV, const float roughness) {
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;
    const float GL = dotNL / (dotNL * (1.0 - k) + k);
    const float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

vec3 fresnel_schlick(const float cos_theta, const vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}
vec3 fresnel_schlick_roughness(const float cosTheta, const vec3 F0, const float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2  saturate(vec2  x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3  saturate(vec3  x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4  saturate(vec4  x) { return clamp(x, vec4(0.0), vec4(1.0)); }

vec3 pbr_specular_contrib(
    const vec3 albedo,
    const vec3 L,
    const vec3 V,
    const vec3 N,
    const vec3 F0,
    const float metallic,
    const float roughness
) {
    vec3 H = normalize (V + L);
    const float dotNH = clamp(dot(N, H), 0.0, 1.0);
    const float dotNV = clamp(dot(N, V), 0.0, 1.0);
    const float dotNL = clamp(dot(N, L), 0.0, 1.0);
    vec3 color = vec3(0.0);
    if (dotNL > 0.0) {
        float D = d_ggx(dotNH, roughness); // D = Normal distribution (Distribution of the microfacets)
        float G = g_geometry_schlick_ggx(dotNL, dotNV, roughness); // G = Geometric shadowing term (Microfacets shadowing)
        vec3 F = fresnel_schlick(dotNV, F0); // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        color += (kD * albedo / kPI + spec) * dotNL;
    }
    return color;
}

vec2 hammersley2d(const uint i, const uint N) {
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = kPI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * kPI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


uvec2 jitterSequence(uint index, uvec2 dimension, uvec2 dispatchId)
{
    uvec2 offset = uvec2(vec2(0.754877669, 0.569840296) * index * dimension);
    uvec2 offsetId = dispatchId + offset;
    offsetId.x = offsetId.x % dimension.x;
    offsetId.y = offsetId.y % dimension.y;

    return offsetId;
}

#endif