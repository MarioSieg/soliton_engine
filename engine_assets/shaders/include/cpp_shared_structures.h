// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.
// ! This header is included from two programming languages: C++ and GLSL, so ensure compatibility!

#ifndef CPP_SHARED_STRUCTURES_H
#define CPP_SHARED_STRUCTURES_H

// Descriptor set indices
#define LU_GLSL_DESCRIPTOR_SET_IDX_PER_FRAME 0
#define LU_GLSL_DESCRIPTOR_SET_IDX_PER_MATERIAL 1
#define LU_GLSL_DESCRIPTOR_SET_IDX_CUSTOM 2

#ifdef __cplusplus // If included from C++
namespace lu::graphics::glsl {
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

// Sun and scene lighting properties. Updated once per frame.
struct perFrameData {
    vec4 camPos;
    vec4 sunDir;
    vec4 sunColor;
};
gpu_check(perFrameData)

#ifdef __cplusplus // If included from C++
}
#endif

#endif
