#version 440

#ifdef PAINT_SOLID
    #define HAS_COLOR 1
#endif

#if defined(PAINT_LINEAR) || defined(PAINT_RADIAL) || defined(PAINT_PATTERN)
    #define HAS_UV0 1
#endif

#ifdef CLAMP_PATTERN
    #define HAS_RECT 1
#endif

#if defined(REPEAT_PATTERN) || defined(MIRRORU_PATTERN) || defined(MIRRORV_PATTERN) || defined(MIRROR_PATTERN)
    #define HAS_RECT 1
    #define HAS_TILE 1
#endif

#ifdef EFFECT_PATH_AA
    #define HAS_COVERAGE 1
#endif

#ifdef EFFECT_SDF
    #define HAS_UV1 1
    #define HAS_ST1 1
    #define SDF_SCALE 7.96875
    #define SDF_BIAS 0.50196078431
    #define SDF_AA_FACTOR 0.65
    #define SDF_BASE_MIN 0.125
    #define SDF_BASE_MAX 0.25
    #define SDF_BASE_DEV -0.65
#endif

#ifdef EFFECT_OPACITY
    #define HAS_UV1 1
#endif

#ifdef EFFECT_SHADOW
    #define HAS_UV1 1
    #define HAS_RECT 1
#endif

#ifdef EFFECT_BLUR
    #define HAS_UV1 1
#endif

#ifdef EFFECT_DOWNSAMPLE
    #define HAS_UV0 1
    #define HAS_UV1 1
    #define HAS_UV2 1
    #define HAS_UV3 1
#endif

#ifdef EFFECT_UPSAMPLE
    #define HAS_COLOR 1
    #define HAS_UV0 1
    #define HAS_UV1 1
#endif

#ifdef EFFECT_CUSTOM
    #define HAS_COLOR 1
    #define HAS_UV0 1
    #define HAS_RECT 1
    #define HAS_IMAGE_POSITION 1
#endif

precision mediump float;
precision mediump int;

#if EFFECT_RGBA || PAINT_LINEAR || PAINT_RADIAL || PAINT_PATTERN
uniform Buffer0
{
#if EFFECT_RGBA
    vec4 rgba;
#endif

#if PAINT_LINEAR || PAINT_PATTERN
    float opacity;
#endif

#if PAINT_RADIAL
    vec4 radialGrad0;
    vec3 radialGrad1;
#endif
};
#endif

#if EFFECT_BLUR
uniform Buffer1
{
    float blend;
};
#endif

#if EFFECT_SHADOW
uniform Buffer1
{
    vec4 shadowColor;
    vec2 shadowOffset;
    float blend;
};
#endif

uniform lowp sampler2D pattern;
uniform lowp sampler2D ramps;
uniform lowp sampler2D image;
uniform lowp sampler2D glyphs;
uniform lowp sampler2D shadow;

layout(location = 0) lowp out vec4 fragColor;

#ifdef HAS_COLOR
    layout(location = 0) flat in lowp vec4 color;
#endif

#ifdef HAS_UV0
    layout(location = 1) in highp vec2 uv0;
#endif

#ifdef HAS_UV1
    layout(location = 2) in highp vec2 uv1;
#endif

#ifdef HAS_UV2
    layout(location = 3) in highp vec2 uv2;
#endif

#ifdef HAS_UV3
    layout(location = 4) in highp vec2 uv3;
#endif

#ifdef HAS_ST1
    layout(location = 5) in highp vec2 st1;
#endif

#ifdef HAS_COVERAGE
    layout(location = 6) in lowp float coverage;
#endif

#ifdef HAS_RECT
    layout(location = 7) flat in highp vec4 rect;
#endif

#ifdef HAS_TILE
    layout(location = 8) flat in highp vec4 tile;
#endif

#ifdef HAS_IMAGE_POSITION
    layout(location = 9) in highp vec4 imagePos;
#endif

lowp vec4 main_brush(highp vec2 uv);
lowp vec4 main_effect();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    /////////////////////////////////////////////////////
    // Fetch paint color and opacity
    /////////////////////////////////////////////////////
    #if PAINT_SOLID
        lowp vec4 paint = color;
        lowp float opacity_ = 1.0f;

    #elif PAINT_LINEAR
        lowp vec4 paint = texture(ramps, uv0);
        lowp float opacity_ = opacity;

    #elif PAINT_RADIAL
        float dd = radialGrad1.x * uv0.x - radialGrad1.y * uv0.y;
        float u = radialGrad0.x * uv0.x + radialGrad0.y * uv0.y + radialGrad0.z *
            sqrt(uv0.x * uv0.x + uv0.y * uv0.y - dd * dd);
        lowp vec4 paint = texture(ramps, vec2(u, radialGrad1.z));
        lowp float opacity_ = radialGrad0.w;

    #elif PAINT_PATTERN
        #if CUSTOM_PATTERN
            lowp vec4 paint = main_brush(uv0);
        #elif CLAMP_PATTERN
            lowp float inside = float(uv0 == clamp(uv0, rect.xy, rect.zw));
            lowp vec4 paint = inside * texture(pattern, uv0);
        #elif REPEAT_PATTERN || MIRRORU_PATTERN || MIRRORV_PATTERN || MIRROR_PATTERN
            highp vec2 uv = (uv0 - tile.xy) / tile.zw;
            #if REPEAT_PATTERN
                uv = fract(uv);
            #elif MIRRORU_PATTERN
                uv.x = abs(uv.x - 2.0 * floor((uv.x - 1.0) / 2.0) - 2.0);
                uv.y = fract(uv.y);
            #elif MIRRORV_PATTERN
                uv.x = fract(uv.x);
                uv.y = abs(uv.y - 2.0 * floor((uv.y - 1.0) / 2.0) - 2.0);
            #else 
                uv = abs(uv - 2.0 * floor((uv - 1.0) / 2.0) - 2.0);
            #endif
            uv = uv * tile.zw + tile.xy;
            lowp float inside = float(uv == clamp(uv, rect.xy, rect.zw));
            lowp vec4 paint = inside * textureGrad(pattern, uv, dFdx(uv0), dFdy(uv0));
        #else
            lowp vec4 paint = texture(pattern, uv0);
        #endif
        lowp float opacity_ = opacity;
    #endif

    /////////////////////////////////////////////////////
    // Apply selected effect
    /////////////////////////////////////////////////////
    #if EFFECT_RGBA
        fragColor = rgba;

    #elif EFFECT_MASK
        fragColor = vec4(1);

    #elif EFFECT_CLEAR
        fragColor = vec4(0);

    #elif EFFECT_PATH
        fragColor = opacity_ * paint;

    #elif EFFECT_PATH_AA
        fragColor = (opacity_ * coverage) * paint;

    #elif EFFECT_OPACITY
        fragColor = texture(image, uv1) * (opacity_ * paint.a);

    #elif EFFECT_SHADOW
        vec2 offset = vec2(shadowOffset.x, shadowOffset.y);
        vec2 uv = clamp(uv1 - offset, rect.xy, rect.zw);
        lowp float alpha = mix(texture(image, uv).a, texture(shadow, uv).a, blend);
        lowp vec4 img = texture(image, clamp(uv1, rect.xy, rect.zw));
        fragColor = (img + (1.0 - img.a) * (shadowColor * alpha)) * (opacity_ * paint.a);

    #elif EFFECT_BLUR
        fragColor = mix(texture(image, uv1), texture(shadow, uv1), blend) * (opacity_ * paint.a);

    #elif EFFECT_SDF
        float distance = SDF_SCALE * (texture(glyphs, uv1).r - SDF_BIAS);
        float gradLen = length(dFdx(st1));
        float scale = 1.0 / gradLen;
        float base = SDF_BASE_DEV * (1.0 - (clamp(scale, SDF_BASE_MIN, SDF_BASE_MAX) - SDF_BASE_MIN) / (SDF_BASE_MAX - SDF_BASE_MIN));
        float range = SDF_AA_FACTOR * gradLen;
        lowp float alpha = smoothstep(base - range, base + range, distance);
        fragColor = (alpha * opacity_) * paint;

    #elif EFFECT_DOWNSAMPLE
        fragColor = (texture(pattern, uv0) + texture(pattern, uv1) + texture(pattern, uv2) + texture(pattern, uv3)) * 0.25;

    #elif EFFECT_UPSAMPLE
        fragColor = mix(texture(image, uv1), texture(pattern, uv0), color.a);

    #elif EFFECT_CUSTOM
        fragColor = main_effect() * (opacity_ * paint.a);

    #else
        #error Effect not defined
    #endif
}
