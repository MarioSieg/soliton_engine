// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
// ! This header is included from two programming languages: C++ and GLSL, so ensure compatibility!

#ifndef CPP_SHARED_STRUCTURES_H
#define CPP_SHARED_STRUCTURES_H

// Descriptor set indices
#define LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME 0
#define LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL 1
#define LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM 2

#ifdef __cplusplus // If included from C++
namespace soliton::graphics::glsl {
    using ivec2 = DirectX::XMINT2;
    using ivec3 = DirectX::XMINT3;
    using ivec4 = DirectX::XMINT4;
    using vec2 = DirectX::XMFLOAT2;
    using vec3 = DirectX::XMFLOAT3;
    using vec4 = DirectX::XMFLOAT4;
    using mat3 = DirectX::XMFLOAT3X3;
    using mat4 = DirectX::XMFLOAT4X4;
    using uint = std::uint32_t;
    #define gpu_check(strut) \
        static_assert(std::is_standard_layout_v<strut> && \
            (sizeof(strut) % (4 * sizeof(float)) == 0));
#else // If included from GLSL
    #define gpu_check(strut)
#endif

// sky params
struct sky_data {
    vec4 sun_luminance;
    vec4 sky_luminance_xyz;
    vec4 sky_luminance;
    vec4 params;
    vec4 perez1;
    vec4 perez2;
    vec4 perez3;
    vec4 perez4;
    vec4 perez5;
};

// Sun and scene lighting properties. Updated once per frame.
struct per_frame_data {
    mat4 viewProj;
    mat4 viewProjInverse;
    vec4 cameraInfo; // x = fov, y = aspect ratio
    vec4 camPos; // w unused for now
    vec4 sunDir; // w unused for now
    vec4 sunColor; // w unused for now
    vec4 ambientColor; // w unused for now

    sky_data sky;
};
gpu_check(per_frame_data)

#ifndef __cplusplus
layout (std140, set = LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME, binding = 0) uniform uniformPerFrameUBO {
    per_frame_data uboPerFrame;
};
#endif

#ifdef __cplusplus // If included from C++
}
#endif

#endif
