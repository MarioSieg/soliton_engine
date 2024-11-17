// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

const float kPI = 3.1415926535897932384626433832795;
const float kTWO_PI = kPI * 2.0;
const float kHALF_PI = kPI * 0.5;

vec3 normalMap(const mat3 tbn, const vec3 n) {
    const vec3 n_map = normalize((n * 2.0 - 1.0));
    return normalize(tbn * n_map);
}

vec3 colorSaturation(const vec3 rgb, const float adjustment) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    const vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

float filmicNoise(const vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// Based omn http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(const vec2 co) {
    const float a = 12.9898;
    const float b = 78.233;
    const float c = 43758.5453;
    const float dt= dot(co.xy ,vec2(a,b));
    const float sn= mod(dt, 3.14);
    return fract(sin(sn) * c);
}

float atan2(vec2 v) {
    return v.x == 0.0 ?
        (1.0 - step(abs(v.y), 0.0)) * sign(v.y) * kPI * 0.5 :
        atan(v.y / v.x) + step(v.x, 0.0) * sign(v.y) * kPI;
}

float saturate(const float x) { return clamp(x, 0.0, 1.0); }
vec2 saturate(const vec2 x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3 saturate(const vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4 saturate(const vec4 x) { return clamp(x, vec4(0.0), vec4(1.0)); }

#endif