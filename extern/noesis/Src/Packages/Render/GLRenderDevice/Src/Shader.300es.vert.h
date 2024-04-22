static const char* Shader_300es_vert = R"(

in vec2 attr_pos;

#ifdef HAS_COLOR
    in lowp vec4 attr_color;
    flat out lowp vec4 color;
#endif

#ifdef HAS_UV0
    in vec2 attr_uv0;
    out vec2 uv0;
#endif

#ifdef HAS_UV1
    in vec2 attr_uv1;
    out vec2 uv1;
#endif

#ifdef DOWNSAMPLE
    out vec2 uv2;
    out vec2 uv3;
#endif

#ifdef SDF
    out vec2 st1;
#endif

#ifdef HAS_RECT
    in vec4 attr_rect;
    flat out vec4 rect;
#endif

#ifdef HAS_TILE
    in vec4 attr_tile;
    flat out vec4 tile;
#endif

#ifdef HAS_COVERAGE
    in lowp float attr_coverage;
    out lowp float coverage;
#endif

#ifdef HAS_IMAGE_POSITION
    in vec4 attr_imagePos;
    out vec4 imagePos;
#endif

uniform float cbuffer0_vs[16];
uniform float cbuffer1_vs[2];

float SRGBToLinear(float v)
{
    if (v <= 0.04045)
    {
      return v * (1.0 / 12.92);
    }
    else
    {
      return pow( v * (1.0 / 1.055) + 0.0521327, 2.4);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    gl_Position = vec4(attr_pos, 0, 1) * mat4
    (
        cbuffer0_vs[0], cbuffer0_vs[1], cbuffer0_vs[2], cbuffer0_vs[3],
        cbuffer0_vs[4], cbuffer0_vs[5], cbuffer0_vs[6], cbuffer0_vs[7],
        cbuffer0_vs[8], cbuffer0_vs[9], cbuffer0_vs[10], cbuffer0_vs[11],
        cbuffer0_vs[12], cbuffer0_vs[13], cbuffer0_vs[14], cbuffer0_vs[15]
    );

#if defined(HAS_COLOR) && defined(SRGB)
    color.r = SRGBToLinear(attr_color.r);
    color.g = SRGBToLinear(attr_color.g);
    color.b = SRGBToLinear(attr_color.b);
    color.a = attr_color.a;
#endif

#if defined(HAS_COLOR) && !defined(SRGB)
    color = attr_color;
#endif

#ifdef DOWNSAMPLE
    uv0 = attr_uv0 + vec2(attr_uv1.x, attr_uv1.y);
    uv1 = attr_uv0 + vec2(attr_uv1.x, -attr_uv1.y);
    uv2 = attr_uv0 + vec2(-attr_uv1.x, attr_uv1.y);
    uv3 = attr_uv0 + vec2(-attr_uv1.x, -attr_uv1.y);
#else
    #ifdef HAS_UV0
        uv0 = attr_uv0;
    #endif
    #ifdef HAS_UV1
        uv1 = attr_uv1;
    #endif
#endif

#ifdef SDF
    st1 = vec2(attr_uv1 * vec2(cbuffer1_vs[0], cbuffer1_vs[1]));
#endif

#ifdef HAS_RECT
    rect = attr_rect;
#endif

#ifdef HAS_TILE
    tile = attr_tile;
#endif

#ifdef HAS_COVERAGE
    coverage = attr_coverage;
#endif

#ifdef HAS_IMAGE_POSITION
    imagePos = attr_imagePos;
#endif
}
)";