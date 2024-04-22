#if defined(TARGET_HLSL)
#include "D3D11RenderDevice/Src/ShaderPS.hlsl"
#define UV_STARTS_AT_TOP 1
#endif

#if defined(TARGET_GLSL) || defined(TARGET_ESSL)
#include "GLRenderDevice/Src/Shader.300es.frag"
#define UV_STARTS_AT_TOP 0
precision highp float;
precision highp int;
#endif

#if defined(TARGET_SPIRV)
#include "VKRenderDevice/Src/Shader.frag"
#define UV_STARTS_AT_TOP 1
precision highp float;
precision highp int;
#endif

#if defined(TARGET_PSSL_ORBIS)
#include "GNMRenderDevice/Src/shader_p.pssl"
#define UV_STARTS_AT_TOP 1
#endif

#if defined(TARGET_PSSL_PROSPERO)
#include "AGCRenderDevice/Src/shader_p.pssl"
#define UV_STARTS_AT_TOP 1
#endif

#if defined(TARGET_NVN)
#include "NVNRenderDevice/Src/Shaders/Shader_fs.inc"
#define UV_STARTS_AT_TOP 0
precision highp float;
precision highp int;
#endif

#if defined(TARGET_HLSL)
#define half4 float4
#define half3 float3
#define half2 float2
#define half float
#define fixed4 float4
#define fixed3 float3
#define fixed2 float2
#define fixed float
float mod(float x, float y) { return x % y; }
#define uniforms cbuffer constants: register(b1)
#endif

#if defined(TARGET_GLSL) || defined(TARGET_ESSL) || defined(TARGET_SPIRV)
#define half4 mediump vec4
#define half3 mediump vec3
#define half2 mediump vec2
#define half mediump float
#define fixed4 lowp vec4
#define fixed3 lowp vec3
#define fixed2 lowp vec2
#define fixed lowp float
#define float4 vec4
#define float3 vec3
#define float2 vec2
#define int4 ivec4
#define int3 ivec3
#define int2 ivec2
#define uint4 uvec4
#define uint3 uvec3
#define uint2 uvec2
#define lerp mix
#define atan2 atan
#define frac fract
#define ddx(v) dFdx(v)
#define ddy(v) dFdy(v)
#define uniforms layout(std140) uniform Buffer1_ps
#endif

#if defined(TARGET_NVN)
#define half4 mediump vec4
#define half3 mediump vec3
#define half2 mediump vec2
#define half mediump float
#define fixed4 lowp vec4
#define fixed3 lowp vec3
#define fixed2 lowp vec2
#define fixed lowp float
#define float4 vec4
#define float3 vec3
#define float2 vec2
#define int4 ivec4
#define int3 ivec3
#define int2 ivec2
#define uint4 uvec4
#define uint3 uvec3
#define uint2 uvec2
#define lerp mix
#define atan2 atan
#define frac fract
#define ddx(v) dFdx(v)
#define ddy(v) dFdy(v)
#define uniforms layout(binding = 1, std140) uniform Buffer1
#endif

#if defined(TARGET_PSSL_ORBIS) || defined(TARGET_PSSL_PROSPERO)
#define half4 float4
#define half3 float3
#define half2 float2
#define half float
#define fixed4 float4
#define fixed3 float3
#define fixed2 float2
#define fixed float
float mod(float x, float y) { return x % y; }
#define uniforms ConstantBuffer Buffer1
#endif
