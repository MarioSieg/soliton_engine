// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

const float kPI = 3.1415926535897932384626433832795;

#include "aces.h"

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
    vec3(0.0, 1.0, 0.0),
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
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

float film_noise(const vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importance_sample_ggx(vec2 Xi, float roughness, vec3 normal) {
    // Maps a 2D point to a hemisphere with spread based on roughness
    float alpha = roughness * roughness;
    float phi = 2.0 * kPI * Xi.x + random(normal.xz) * 0.1;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // Tangent space
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(up, normal));
    vec3 tangentY = normalize(cross(normal, tangentX));

    // Convert to world Space
    return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

// Normal Distribution function
float d_ggx(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(kPI * denom*denom);
}

vec2 hammersley2d(uint i, uint N) {
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return vec2(float(i) /float(N), rdi);
}

// high frequency dither pattern appearing almost random without banding steps
// note: from "NEXT GENERATION POST PROCESSING IN CALL OF DUTY: ADVANCED WARFARE"
//      http://advances.realtimerendering.com/s2014/index.html
float interleaved_gradient_noise(vec2 uv, float frameId) {
    uv += frameId * (vec2(47.0, 17.0) * 0.695f); // magic values are found by experimentation
    const vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

// 3D random number generator inspired by PCGs (permuted congruential generator)
// Using a Feistel cipher in place of the usual xor shift permutation step
// p = 3D integer coordinate
// Returns three elements w/ 16 random bits each (0-0xffff).
// ~8 ALU operations for result.x    (7 mad, 1 >>)
// ~10 ALU operations for result.xy  (8 mad, 2 >>)
// ~12 ALU operations for result.xyz (9 mad, 3 >>)
uvec3 rand3DPCG16(ivec3 p) {
    // taking a signed int then reinterpreting as unsigned gives good behavior for negatives
    uvec3 v = uvec3(p);

    // Linear congruential step. These LCG constants are from Numerical Recipies
    // For additional #'s, PCG would do multiple LCG steps and scramble each on output
    // So v here is the RNG state
    v = v * 1664525u + 1013904223u;

    // PCG uses xorshift for the final shuffle, but it is expensive (and cheap
    // versions of xorshift have visible artifacts). Instead, use simple MAD Feistel steps
    //
    // Feistel ciphers divide the state into separate parts (usually by bits)
    // then apply a series of permutation steps one part at a time. The permutations
    // use a reversible operation (usually ^) to part being updated with the result of
    // a permutation function on the other parts and the key.
    //
    // In this case, I'm using v.x, v.y and v.z as the parts, using + instead of ^ for
    // the combination function, and just multiplying the other two parts (no key) for
    // the permutation function.
    //
    // That gives a simple mad per round.
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;
    v.x += v.y*v.z;
    v.y += v.z*v.x;
    v.z += v.x*v.y;

    // only top 16 bits are well shuffled
    return v >> 16u;
}

// Quad schedule style, fake pixel shader dispatch style.
// Input-> [0, 63]
//
// Output:
//  00 01 08 09 10 11 18 19
//  02 03 0a 0b 12 13 1a 1b
//  04 05 0c 0d 14 15 1c 1d
//  06 07 0e 0f 16 17 1e 1f
//  20 21 28 29 30 31 38 39
//  22 23 2a 2b 32 33 3a 3b
//  24 25 2c 2d 34 35 3c 3d
//  26 27 2e 2f 36 37 3e 3f
uvec2 remap8x8(uint lane) // gl_LocalInvocationIndex in 8x8 threadgroup.
{
    return uvec2(
        (((lane >> 2) & 0x0007) & 0xFFFE) | lane & 0x0001,
        ((lane >> 1) & 0x0003) | (((lane >> 3) & 0x0007) & 0xFFFC)
    );
}

float mean(vec2 v) { return dot(v, vec2(1.0f / 2.0f)); }
float mean(vec3 v) { return dot(v, vec3(1.0f / 3.0f)); }
float mean(vec4 v) { return dot(v, vec4(1.0f / 4.0f)); }

float hsum(vec2 v) { return v.x + v.y; }
float hsum(vec3 v) { return v.x + v.y + v.z; }
float hsum(vec4 v) { return v.x + v.y + v.z + v.w; }

float max3(vec3 xyz) { return max(xyz.x, max(xyz.y, xyz.z)); }
float max4(vec4 xyzw) { return max(xyzw.x, max(xyzw.y, max(xyzw.z, xyzw.w))); }

float min3(vec3 xyz) { return min(xyz.x, min(xyz.y, xyz.z)); }
float min4(vec4 xyzw) { return min(xyzw.x, min(xyzw.y, min(xyzw.z, xyzw.w))); }

float sat(float x) { return clamp(x, 0.0, 1.0); }
vec2 sat(vec2 x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3 sat(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4 sat(vec4 x) { return clamp(x, vec4(0.0), vec4(1.0)); }

// Saturated range, [0, 1]
bool is_saturated(float x) { return x >= 0.0f && x <= 1.0f; }
bool is_saturated(vec2 x) { return is_saturated(x.x) && is_saturated(x.y); }
bool is_saturated(vec3 x) { return is_saturated(x.x) && is_saturated(x.y) && is_saturated(x.z);}
bool is_saturated(vec4 x) { return is_saturated(x.x) && is_saturated(x.y) && is_saturated(x.z) && is_saturated(x.w);}

// On range, [minV, maxV]
bool on_range(float x, float minV, float maxV) { return x >= minV && x <= maxV;}
bool on_range(vec2 x, vec2 minV, vec2 maxV) { return on_range(x.x, minV.x, maxV.x) && on_range(x.y, minV.y, maxV.y);}
bool on_range(vec3 x, vec3 minV, vec3 maxV) { return on_range(x.x, minV.x, maxV.x) && on_range(x.y, minV.y, maxV.y) && on_range(x.z, minV.z, maxV.z);}
bool on_range(vec4 x, vec4 minV, vec4 maxV) { return on_range(x.x, minV.x, maxV.x) && on_range(x.y, minV.y, maxV.y) && on_range(x.z, minV.z, maxV.z) && on_range(x.w, minV.w, maxV.w);}

// Rounds value to the nearest multiple of 8
uvec2 roundUp8(uvec2 value) {
    uvec2 roundDown = value & ~0x7;
    return (roundDown == value) ? value : value + 8;
}

float luminance(vec3 color) {
    // human eye aware lumiance function.
    return dot(color, vec3(0.299, 0.587, 0.114));
}

float luminanceRec709(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// Build one TBN matrix from normal input.
mat3 build_tbn(vec3 N) {
    vec3 U;
    if (abs(N.z) > 0.0) {
        float k = sqrt(N.y * N.y + N.z * N.z);
        U.x = 0.0;
        U.y = -N.z / k;
        U.z = N.y / k;
    } else {
        float k = sqrt(N.x * N.x + N.y * N.y);
        U.x = N.y / k;
        U.y = -N.x / k;
        U.z = 0.0;
    }
    mat3 TBN = mat3(U, cross(N, U), N);
    return transpose(TBN);
}

float uniform_phase() {
    return 1.0f / (4.0f * kPI);
}

// https://www.shadertoy.com/view/Mtc3Ds
// rayleigh phase function.
float rayleigh_phase(float cosTheta) {
    const float factor = 3.0f / (16.0f * kPI);
    return factor * (1.0f + cosTheta * cosTheta);
}

// Crazy light intensity.
float henyey_greenstein(float cosTheta, float g) {
    float gg = g * g;
    return (1. - gg) / pow(1. + gg - 2. * g * cosTheta, 1.5);
}

// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float hg_phase(float g, float cosTheta) {
    float numer = 1.0f - g * g;
    float denom = 1.0f + g * g + 2.0f * g * cosTheta;
    return numer / (4.0f * kPI * denom * sqrt(denom));
}

float dual_lob_phase(float g0, float g1, float w, float cosTheta) {
    return mix(hg_phase(g0, cosTheta), hg_phase(g1, cosTheta), w);
}

float beers_law(float density, float stepLength, float densityScale) {
    return exp(-density * stepLength * densityScale);
}

// From https://www.shadertoy.com/view/4sjBDG
float numerical_mie_fit(float costh) {
    // This function was optimized to minimize (delta*delta)/reference in order to capture
    // the low intensity behavior.
    float bestParams[10];
    bestParams[0]=9.805233e-06;
    bestParams[1]=-6.500000e+01;
    bestParams[2]=-5.500000e+01;
    bestParams[3]=8.194068e-01;
    bestParams[4]=1.388198e-01;
    bestParams[5]=-8.370334e+01;
    bestParams[6]=7.810083e+00;
    bestParams[7]=2.054747e-03;
    bestParams[8]=2.600563e-02;
    bestParams[9]=-4.552125e-12;
    float p1 = costh + bestParams[3];
    vec4 expValues = exp(vec4(bestParams[1] *costh+bestParams[2], bestParams[5] *p1*p1, bestParams[6] *costh, bestParams[9] *costh));
    vec4 expValWeight= vec4(bestParams[0], bestParams[4], bestParams[7], bestParams[8]);
    return dot(expValues, expValWeight);
}

// Relative error : ~3.4% over full
// Precise format : ~small float
// 2 ALU
float rsqrt_fast(float x) {
    int i = floatBitsToInt(x);
    i = 0x5f3759df - (i >> 1);
    return intBitsToFloat (i);
}

// max absolute error 9.0x10^-3
// Eberly's polynomial degree 1 - respect bounds
// 4 VGPR, 12 FR (8 FR, 1 QR), 1 scalar
// input [-1, 1] and output [0, kPI]
float acosFast(float inX) {
    float x = abs(inX);
    float res = -0.156583f * x + (0.5 * kPI);
    res *= sqrt(1.0f - x);
    return (inX >= 0) ? res : kPI - res;
}

// Approximates acos(x) with a max absolute error of 9.0x10^-3.
// Input [0, 1]
float acos_fast_positive(float x) {
    float p = -0.1565827f * x + 1.570796f;
    return p * sqrt(1.0 - x);
}

float whang_hash_noise(uint u, uint v, uint s) {
    uint seed = (u*1664525u + v) + s;
    seed  = (seed ^ 61u) ^(seed >> 16u);
    seed *= 9u;
    seed  = seed ^(seed >> 4u);
    seed *= uint(0x27d4eb2d);
    seed  = seed ^(seed >> 15u);
    float value = float(seed) / (4294967296.0);
    return value;
}

// Simple hash uint.
// from niagara stream. see https://www.youtube.com/watch?v=BR2my8OE1Sc
uint hash(uint a) {
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

// Simple hash color from uint value.
// from niagara stream. see https://www.youtube.com/watch?v=BR2my8OE1Sc
vec3 hash_color(uint i) {
    uint h = hash(i);
    return vec3(float(h & 255), float((h >> 8) & 255), float((h >> 16) & 255)) / 255.0;
}

// return intersect t, negative meaning no intersection.
float line_plane_intersect(vec3 lineStart, vec3 lineDirection, vec3 planeP, vec3 planeNormal) {
    float ndl = dot(lineDirection, planeNormal);
    return ndl != 0.0f ? dot((planeP - lineStart), planeNormal) / ndl : -1.0f;
}

// world space box intersect.
// Axis is y up, negative meaning no intersection.
float box_line_intersect_ws(vec3 lineStart, vec3 lineDirection, vec3 bboxMin, vec3 bboxMax) {
    const float kMaxDefault = 9e10f;
    const float kMaxDiff = 9e9f;
    float t0 = line_plane_intersect(lineStart, lineDirection, bboxMin, vec3(-1.0f, 0.0f, 0.0f)); t0 = t0 < 0.0f ? kMaxDefault : t0;
    float t1 = line_plane_intersect(lineStart, lineDirection, bboxMax, vec3( 1.0f, 0.0f, 0.0f)); t1 = t1 < 0.0f ? kMaxDefault : t1;
    float t2 = line_plane_intersect(lineStart, lineDirection, bboxMax, vec3( 0.0f, 1.0f, 0.0f)); t2 = t2 < 0.0f ? kMaxDefault : t2;
    float t3 = line_plane_intersect(lineStart, lineDirection, bboxMin, vec3(0.0f, -1.0f, 0.0f)); t3 = t3 < 0.0f ? kMaxDefault : t3;
    float t4 = line_plane_intersect(lineStart, lineDirection, bboxMin, vec3(0.0f, 0.0f, -1.0f)); t4 = t4 < 0.0f ? kMaxDefault : t4;
    float t5 = line_plane_intersect(lineStart, lineDirection, bboxMax, vec3( 0.0f, 0.0f, 1.0f)); t5 = t5 < 0.0f ? kMaxDefault : t5;
    float tMin = t0;
    tMin = min(tMin, t1);
    tMin = min(tMin, t2);
    tMin = min(tMin, t3);
    tMin = min(tMin, t4);
    tMin = min(tMin, t5);
    return tMin < kMaxDiff ? tMin : -1.0f;
}

float intersect_dir_plane_one_sided(vec3 dir, vec3 normal, vec3 pt) {
    float d = -dot(pt, normal);
    float t = d / max(1e-5f, -dot(dir, normal));
    return t;
}

// From Open Asset Import Library
// https://github.com/assimp/assimp/blob/master/include/assimp/matrix3x3.inl
mat3 rotFromToMatrix(vec3 from, vec3 to) {
    float e = dot(from, to);
    float f = abs(e);
    if (f > 1.0f - 0.0003f) {
        return mat3(
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f
        );
    }
    vec3 v   = cross(from, to);
    float h    = 1.f / (1.f + e);      /* optimization by Gottfried Chen */
    float hvx  = h * v.x;
    float hvz  = h * v.z;
    float hvxy = hvx * v.y;
    float hvxz = hvx * v.z;
    float hvyz = hvz * v.y;

    mat3 mtx;
    mtx[0][0] = e + hvx * v.x;
    mtx[0][1] = hvxy - v.z;
    mtx[0][2] = hvxz + v.y;

    mtx[1][0] = hvxy + v.z;
    mtx[1][1] = e + h * v.y * v.y;
    mtx[1][2] = hvyz - v.x;

    mtx[2][0] = hvxz - v.y;
    mtx[2][1] = hvyz + v.x;
    mtx[2][2] = e + hvz * v.z;

    return transpose(mtx);
}

float atan2(vec2 v) {
    return v.x == 0.0 ?
       (1.0 - step(abs(v.y), 0.0)) * sign(v.y) * kPI * 0.5 :
       atan(v.y / v.x) + step(v.x, 0.0) * sign(v.y) * kPI;
}

// Gamma curve encode to srgb.
vec3 encode_srgb_gamma(vec3 linearRGB) {
    // Most PC Monitor is 2.2 Gamma, maybe this function is enough.
    // return pow(linearRGB, vec3(1.0 / 2.2));

    // TV encode Rec709 encode.
    vec3 a = 12.92 * linearRGB;
    vec3 b = 1.055 * pow(linearRGB, vec3(1.0 / 2.4)) - 0.055;
    vec3 c = step(vec3(0.0031308), linearRGB);
    return mix(a, b, c);
}

#endif