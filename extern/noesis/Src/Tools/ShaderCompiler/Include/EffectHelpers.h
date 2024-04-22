#define EFFECT_CUSTOM 1
#include "BaseShader.h"

//// Returns the current input coordinate. As the effect may be generated inside an texture atlas,
//// shaders shouldn't take any dependencies on how this value is calculated. It should use it only
//// to the pixel shader's input. For rest of cases GetNormalizedInputCoordinate() is recommended
// float2 GetInputCoordinate()

//// Returns the current normalized input coordinates in the range 0 to 1
// float2 GetNormalizedInputCoordinate()

//// Returns the current image position in pixels
// float2 GetImagePosition()

//// Returns the color at the current input coordinates
// fixed4 GetInput()

//// Samples input at position uv
// fixed4 SampleInput(float2 uv)

//// Samples input at an offset in pixels from the input coordinate
// fixed4 SampleInputAtOffset(float2 offset)

//// Samples input at an absolute scene position in pixels
// fixed4 SampleInputAtPosition(float2 pos)

#if defined(TARGET_HLSL)
float2 GetInputCoordinate() { return i_.uv0; }
float2 GetNormalizedInputCoordinate() { return float2((i_.uv0.x - i_.rect.x) / (i_.rect.z - i_.rect.x), (i_.uv0.y - i_.rect.y) / (i_.rect.w - i_.rect.y)); }
float2 GetImagePosition() { return i_.imagePos.xy; }
fixed4 GetInput() { return pattern.Sample(patternSampler, i_.uv0); }
fixed4 SampleInput(float2 uv) { return pattern.Sample(patternSampler, uv); }
fixed4 SampleInputAtOffset(float2 offset) { return SampleInput(clamp(i_.uv0 + offset * i_.imagePos.zw, i_.rect.xy, i_.rect.zw)); }
fixed4 SampleInputAtPosition(float2 pos) { return SampleInputAtOffset(pos - i_.imagePos.xy); }
#endif

#if defined(TARGET_GLSL) || defined(TARGET_ESSL) || defined(TARGET_SPIRV)
vec2 GetInputCoordinate() { return uv0; }
vec2 GetNormalizedInputCoordinate() { return vec2((uv0.x - rect.x) / (rect.z - rect.x), (uv0.y - rect.y) / (rect.w - rect.y)); }
vec2 GetImagePosition() { return imagePos.xy; }
fixed4 GetInput() { return texture(pattern, uv0); }
fixed4 SampleInput(vec2 uv) { return texture(pattern, uv); }
fixed4 SampleInputAtOffset(vec2 offset) { return SampleInput(clamp(uv0 + offset * imagePos.zw, rect.xy, rect.zw)); }
fixed4 SampleInputAtPosition(vec2 pos) { return SampleInputAtOffset(pos - imagePos.xy); }
#endif

#if defined(TARGET_NVN)
vec2 GetInputCoordinate() { return uv0; }
vec2 GetNormalizedInputCoordinate() { return vec2((uv0.x - rect.x) / (rect.z - rect.x), (uv0.y - rect.y) / (rect.w - rect.y)); }
vec2 GetImagePosition() { return imagePos.xy; }
fixed4 GetInput() { return texture(sampler2D(pattern, patternSampler), uv0); }
fixed4 SampleInput(vec2 uv) { return texture(sampler2D(pattern, patternSampler), uv); }
fixed4 SampleInputAtOffset(vec2 offset) { return SampleInput(clamp(uv0 + offset * imagePos.zw, rect.xy, rect.zw)); }
fixed4 SampleInputAtPosition(vec2 pos) { return SampleInputAtOffset(pos - imagePos.xy); }
#endif

#if defined(TARGET_PSSL_ORBIS) || defined(TARGET_PSSL_PROSPERO)
float2 GetInputCoordinate() { return i_.uv0; }
float2 GetNormalizedInputCoordinate() { return float2((i_.uv0.x - i_.rect.x) / (i_.rect.z - i_.rect.x), (i_.uv0.y - i_.rect.y) / (i_.rect.w - i_.rect.y)); }
float2 GetImagePosition() { return i_.imagePos.xy; }
fixed4 GetInput() { return pattern.Sample(patternSampler, i_.uv0); }
fixed4 SampleInput(float2 uv) { return pattern.Sample(patternSampler, uv); }
fixed4 SampleInputAtOffset(float2 offset) { return SampleInput(clamp(i_.uv0 + offset * i_.imagePos.zw, i_.rect.xy, i_.rect.zw)); }
fixed4 SampleInputAtPosition(float2 pos) { return SampleInputAtOffset(pos - i_.imagePos.xy); }
#endif
