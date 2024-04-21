#define PAINT_PATTERN 1
#define CUSTOM_PATTERN 1
#include "BaseShader.h"

//// Samples image texture at position uv
// fixed4 SampleImage(float2 uv)

//// Samples secondary image texture at position uv
// fixed4 SampleImage2(float2 uv)

#if defined(TARGET_HLSL)
fixed4 SampleImage(float2 uv) { return pattern.Sample(patternSampler, uv); }
fixed4 SampleImage2(float2 uv) { return shadow.Sample(shadowSampler, uv); }
#endif

#if defined(TARGET_GLSL) || defined(TARGET_ESSL) || defined(TARGET_SPIRV)
fixed4 SampleImage(vec2 uv) { return texture(pattern, uv); }
fixed4 SampleImage2(vec2 uv) { return texture(shadow, uv); }
#endif

#if defined(TARGET_NVN)
fixed4 SampleImage(vec2 uv) { return texture(sampler2D(pattern, patternSampler), uv); }
fixed4 SampleImage2(vec2 uv) { return texture(sampler2D(shadow, shadowSampler), uv); }

#endif

#if defined(TARGET_PSSL_ORBIS) || defined(TARGET_PSSL_PROSPERO)
fixed4 SampleImage(float2 uv) { return pattern.Sample(patternSampler, uv); }
fixed4 SampleImage2(float2 uv) { return shadow.Sample(shadowSampler, uv); }
#endif
