#version 440

#ifdef STEREO_LAYER
#extension GL_ARB_shader_viewport_layer_array : require
#endif

#ifdef STEREO_MULTIVIEW
#extension GL_EXT_multiview : require
#endif

// Input
layout(location = 0) in highp vec2 attr_pos;
#ifdef HAS_COLOR
    layout(location = 1) in lowp vec4 attr_color;
#endif
#ifdef HAS_UV0
    layout(location = 2) in highp vec2 attr_uv0;
#endif
#ifdef HAS_UV1
    layout(location = 3) in highp vec2 attr_uv1;
#endif
#ifdef HAS_COVERAGE
    layout(location = 4) in lowp float attr_coverage;
#endif
#ifdef HAS_RECT
    layout(location = 5) in highp vec4 attr_rect;
#endif
#ifdef HAS_TILE
    layout(location = 6) in highp vec4 attr_tile;
#endif
#ifdef HAS_IMAGE_POSITION
    layout(location = 7) in highp vec4 attr_imagePos;
#endif

// Output
#ifdef HAS_COLOR
    layout(location = 0) flat out lowp vec4 color;
#endif
#ifdef HAS_UV0
    layout(location = 1) out highp vec2 uv0;
#endif
#ifdef HAS_UV1
    layout(location = 2) out highp vec2 uv1;
#endif
#ifdef DOWNSAMPLE
    layout(location = 3) out highp vec2 uv2;
    layout(location = 4) out highp vec2 uv3;
#endif
#ifdef SDF
    layout(location = 5) out highp vec2 st1;
#endif
#ifdef HAS_COVERAGE
    layout(location = 6) out lowp float coverage;
#endif
#ifdef HAS_RECT
    layout(location = 7) flat out highp vec4 rect;
#endif
#ifdef HAS_TILE
    layout(location = 8) flat out highp vec4 tile;
#endif
#ifdef HAS_IMAGE_POSITION
    layout(location = 9) out highp vec4 imagePos;
#endif

uniform Buffer0
{
  #if STEREO_LAYER || STEREO_MULTIVIEW
    highp mat4 projectionMtx[2];
  #else
    highp mat4 projectionMtx;
  #endif
};

uniform Buffer1
{
    highp vec2 textureDimensions;
};

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
#if (STEREO_LAYER || STEREO_MULTIVIEW)
  #ifdef STEREO_LAYER
    gl_Position = vec4(attr_pos, 0, 1) * projectionMtx[gl_InstanceIndex];
    gl_Layer = gl_InstanceIndex;
  #else
    gl_Position = vec4(attr_pos, 0, 1) * projectionMtx[gl_ViewIndex];
  #endif
#else
    gl_Position = vec4(attr_pos, 0, 1) * projectionMtx;
#endif

#ifdef HAS_COLOR
  #ifdef SRGB
    color.r = SRGBToLinear(attr_color.r);
    color.g = SRGBToLinear(attr_color.g);
    color.b = SRGBToLinear(attr_color.b);
    color.a = attr_color.a;
  #else
    color = attr_color;
  #endif
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
    st1 = vec2(attr_uv1 * textureDimensions);
#endif

#ifdef HAS_COVERAGE
    coverage = attr_coverage;
#endif

#ifdef HAS_RECT
    rect = attr_rect;
#endif

#ifdef HAS_TILE
    tile = attr_tile;
#endif

#ifdef HAS_IMAGE_POSITION
    imagePos = attr_imagePos;
#endif
}