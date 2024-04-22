static const char* Shader_100es_frag = R"(

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
    #extension GL_OES_standard_derivatives : enable
    #extension GL_EXT_shader_texture_lod : enable
    #define HAS_RECT 1
    #define HAS_TILE 1
#endif

#ifdef EFFECT_PATH_AA
    #define HAS_COVERAGE 1
#endif

#ifdef EFFECT_SDF
    #extension GL_OES_standard_derivatives : enable
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

precision mediump float;

#ifndef GL_FRAGMENT_PRECISION_HIGH
    #define highp mediump
#endif

uniform float cbuffer0_ps[8];
uniform float cbuffer1_ps[128];

uniform sampler2D pattern;
uniform sampler2D ramps;
uniform sampler2D image;
uniform sampler2D glyphs;
uniform sampler2D shadow;

#ifdef HAS_COLOR
    varying lowp vec4 color;
#endif

#ifdef HAS_UV0
    varying highp vec2 uv0;
#endif

#ifdef HAS_UV1
    varying highp vec2 uv1;
#endif

#ifdef HAS_UV2
    varying highp vec2 uv2;
#endif

#ifdef HAS_UV3
    varying highp vec2 uv3;
#endif

#ifdef HAS_ST1
    varying highp vec2 st1;
#endif

#ifdef HAS_RECT
    varying highp vec4 rect;
#endif

#ifdef HAS_TILE
    varying highp vec4 tile;
#endif

#ifdef HAS_COVERAGE
    varying lowp float coverage;
#endif

#if !GL_EXT_shader_texture_lod
    #define texture2DGradEXT(s, p, dPdx, dPdy) texture2D(s, p)
#endif

lowp vec4 main_brush(highp vec2 uv);

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    /////////////////////////////////////////////////////
    // Fetch paint color and opacity
    /////////////////////////////////////////////////////
    #if defined(PAINT_SOLID)
        lowp vec4 paint = color;
        lowp float opacity_ = 1.0;
    #endif

    #if defined(PAINT_LINEAR)
        lowp vec4 paint = texture2D(ramps, uv0);
        lowp float opacity_ = cbuffer0_ps[0];
    #endif

    #if defined(PAINT_RADIAL)
        float dd = cbuffer0_ps[4] * uv0.x - cbuffer0_ps[5] * uv0.y;
        float u = cbuffer0_ps[0] * uv0.x + cbuffer0_ps[1] * uv0.y + cbuffer0_ps[2] * 
            sqrt(uv0.x * uv0.x + uv0.y * uv0.y - dd * dd);
        lowp vec4 paint = texture2D(ramps, vec2(u, cbuffer0_ps[6]));
        lowp float opacity_ = cbuffer0_ps[3];
    #endif

    #if defined(PAINT_PATTERN)
        #if defined(CUSTOM_PATTERN)
            lowp vec4 paint = main_brush(uv0);
        #endif
        #if defined(CLAMP_PATTERN)
            lowp float inside = float(uv0 == clamp(uv0, rect.xy, rect.zw));
            lowp vec4 paint = inside * texture2D(pattern, uv0);
        #endif
        #if defined(REPEAT_PATTERN) || defined(MIRRORU_PATTERN) || defined(MIRRORV_PATTERN) || defined(MIRROR_PATTERN)
            highp vec2 uv = (uv0 - tile.xy) / tile.zw;
            #if defined(REPEAT_PATTERN)
                uv = fract(uv);
            #endif
            #if defined(MIRRORU_PATTERN)
                uv.x = abs(uv.x - 2.0 * floor((uv.x - 1.0) / 2.0) - 2.0);
                uv.y = fract(uv.y);
            #endif
            #if defined(MIRRORV_PATTERN)
                uv.x = fract(uv.x);
                uv.y = abs(uv.y - 2.0 * floor((uv.y - 1.0) / 2.0) - 2.0);
            #endif
            #if defined(MIRROR_PATTERN)
                uv = abs(uv - 2.0 * floor((uv - 1.0) / 2.0) - 2.0);
            #endif
            uv = uv * tile.zw + tile.xy;
            lowp float inside = float(uv == clamp(uv, rect.xy, rect.zw));
            lowp vec4 paint = inside * texture2DGradEXT(pattern, uv, dFdx(uv0), dFdy(uv0));
        #endif
        #if !defined(CUSTOM_PATTERN) && !defined(CLAMP_PATTERN) && !defined(REPEAT_PATTERN) && !defined(MIRRORU_PATTERN) && !defined(MIRRORV_PATTERN) && !defined(MIRROR_PATTERN)
            lowp vec4 paint = texture2D(pattern, uv0);
        #endif
        lowp float opacity_ = cbuffer0_ps[0];
    #endif

    /////////////////////////////////////////////////////
    // Apply selected effect
    /////////////////////////////////////////////////////
    #if defined(EFFECT_RGBA)
        gl_FragColor = vec4(cbuffer0_ps[0], cbuffer0_ps[1], cbuffer0_ps[2], cbuffer0_ps[3]);
    #endif

    #if defined(EFFECT_MASK)
        gl_FragColor = vec4(1);
    #endif

    #if defined(EFFECT_CLEAR)
        gl_FragColor = vec4(0);
    #endif

    #if defined(EFFECT_PATH)
        gl_FragColor = opacity_ * paint;
    #endif

    #if defined(EFFECT_PATH_AA)
        gl_FragColor = (opacity_ * coverage) * paint;
    #endif

    #if defined(EFFECT_OPACITY)
        gl_FragColor = texture2D(image, uv1) * (opacity_ * paint.a);
    #endif

    #if defined(EFFECT_SHADOW)
        lowp vec4 shadowColor = vec4(cbuffer1_ps[0], cbuffer1_ps[1], cbuffer1_ps[2], cbuffer1_ps[3]);
        vec2 offset = vec2(cbuffer1_ps[4], -cbuffer1_ps[5]);
        vec2 uv = clamp(uv1 - offset, rect.xy, rect.zw);
        lowp float alpha = mix(texture2D(image, uv).a, texture2D(shadow, uv).a, cbuffer1_ps[6]);
        lowp vec4 img = texture2D(image, clamp(uv1, rect.xy, rect.zw));
        gl_FragColor = (img + (1.0 - img.a) * (shadowColor * alpha)) * (opacity_ * paint.a);
    #endif

    #if defined(EFFECT_BLUR)
        gl_FragColor = mix(texture2D(image, uv1), texture2D(shadow, uv1), cbuffer1_ps[0]) * (opacity_ * paint.a);
    #endif

    #if defined(EFFECT_SDF)
        float distance = SDF_SCALE * (texture2D(glyphs, uv1).r - SDF_BIAS);
        float gradLen = length(dFdx(st1));
        float scale = 1.0 / gradLen;
        float base = SDF_BASE_DEV * (1.0 - (clamp(scale, SDF_BASE_MIN, SDF_BASE_MAX) - SDF_BASE_MIN) / (SDF_BASE_MAX - SDF_BASE_MIN));
        float range = SDF_AA_FACTOR * gradLen;
        lowp float alpha = smoothstep(base - range, base + range, distance);
        gl_FragColor = (alpha * opacity_) * paint;
    #endif

    #if defined(EFFECT_DOWNSAMPLE)
        gl_FragColor = (texture2D(pattern, uv0) + texture2D(pattern, uv1) + texture2D(pattern, uv2) + texture2D(pattern, uv3)) * 0.25;
    #endif

    #if defined(EFFECT_UPSAMPLE)
        gl_FragColor = mix(texture2D(image, uv1), texture2D(pattern, uv0), color.a);
    #endif
}

)";