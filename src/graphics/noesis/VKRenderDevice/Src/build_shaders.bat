@echo off

if exist Shaders.tar del Shaders.tar
if exist Shaders.h del Shaders.h

:: Note that we are packing bindings in a single descriptor set without holes using auto-binding
:: Vertex shaders can have one or two buffers, we need to set the pixel shader offset based on that

:: Vertex Shaders

CALL :glslc 0 Pos_VS Shader.vert
CALL :glslc 0 PosColor_VS "-DHAS_COLOR=1" Shader.vert
CALL :glslc 0 PosTex0_VS "-DHAS_UV0=1" Shader.vert
CALL :glslc 0 PosTex0Rect_VS "-DHAS_UV0=1" "-DHAS_RECT=1" Shader.vert
CALL :glslc 0 PosTex0RectTile_VS "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_TILE=1" Shader.vert
CALL :glslc 0 PosColorCoverage_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" Shader.vert
CALL :glslc 0 PosTex0Coverage_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRect_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRectTile_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" "-DHAS_TILE=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_SDF_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSDF=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_SDF_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSDF=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_SDF_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSDF=1" Shader.vert
CALL :glslc 0 PosColorTex1_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_VS "-DHAS_UV0=1" "-DHAS_UV1=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_Downsample_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DDOWNSAMPLE=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" Shader.vert

CALL :glslc 0 PosColor_SRGB_VS "-DHAS_COLOR=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorCoverage_SRGB_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_SRGB_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorTex1_SRGB_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_SRGB_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_SRGB_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSRGB=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_SRGB_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" "-DSRGB=1" Shader.vert

:: Vertex Shaders (GL_EXT_multiview)

CALL :glslc 0 Pos_Multiview_VS "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColor_Multiview_VS "-DHAS_COLOR" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0_Multiview_VS "-DHAS_UV0=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Rect_Multiview_VS "-DHAS_UV0=1" "-DHAS_RECT=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0RectTile_Multiview_VS "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorCoverage_Multiview_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Coverage_Multiview_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRect_Multiview_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRectTile_Multiview_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_SDF_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSDF=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_SDF_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSDF=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_SDF_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSDF=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_Downsample_Multiview_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DDOWNSAMPLE=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" "-DSTEREO_MULTIVIEW=1" Shader.vert

CALL :glslc 0 PosColor_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorCoverage_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_SRGB_Multiview_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" "-DSRGB=1" "-DSTEREO_MULTIVIEW=1" Shader.vert

:: Vertex Shaders (GL_ARB_shader_viewport_layer_array)

CALL :glslc 0 Pos_Layer_VS "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColor_Layer_VS "-DHAS_COLOR" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0_Layer_VS "-DHAS_UV0=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Rect_Layer_VS "-DHAS_UV0=1" "-DHAS_RECT=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0RectTile_Layer_VS "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorCoverage_Layer_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Coverage_Layer_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRect_Layer_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0CoverageRectTile_Layer_VS "-DHAS_UV0=1" "-DHAS_COVERAGE=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_SDF_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSDF=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_SDF_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSDF=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_SDF_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSDF=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1Rect_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1RectTile_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DHAS_TILE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosTex0Tex1_Downsample_Layer_VS "-DHAS_UV0=1" "-DHAS_UV1=1" "-DDOWNSAMPLE=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" "-DSTEREO_LAYER=1" Shader.vert

CALL :glslc 0 PosColor_SRGB_Layer_VS "-DHAS_COLOR=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorCoverage_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_COVERAGE=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1_SDF_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSDF=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex0Tex1_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_UV1=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex1Rect_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV1=1" "-DHAS_RECT=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert
CALL :glslc 0 PosColorTex0RectImagePos_SRGB_Layer_VS "-DHAS_COLOR=1" "-DHAS_UV0=1" "-DHAS_RECT=1" "-DHAS_IMAGE_POSITION=1" "-DSRGB=1" "-DSTEREO_LAYER=1" Shader.vert

:: Pixel Shaders

CALL :glslc 1 RGBA_PS "-DEFFECT_RGBA=1" Shader.frag
CALL :glslc 1 Mask_PS "-DEFFECT_MASK=1" Shader.frag
CALL :glslc 1 Clear_PS "-DEFFECT_CLEAR=1" Shader.frag

CALL :glslc 1 Path_Solid_PS "-DEFFECT_PATH=1" "-DPAINT_SOLID=1" Shader.frag
CALL :glslc 1 Path_Linear_PS "-DEFFECT_PATH=1" "-DPAINT_LINEAR=1" Shader.frag
CALL :glslc 1 Path_Radial_PS "-DEFFECT_PATH=1" "-DPAINT_RADIAL=1" Shader.frag
CALL :glslc 1 Path_Pattern_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" Shader.frag
CALL :glslc 1 Path_Pattern_Clamp_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" "-DCLAMP_PATTERN=1" Shader.frag
CALL :glslc 1 Path_Pattern_Repeat_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" "-DREPEAT_PATTERN=1" Shader.frag
CALL :glslc 1 Path_Pattern_MirrorU_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" "-DMIRRORU_PATTERN=1" Shader.frag
CALL :glslc 1 Path_Pattern_MirrorV_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" "-DMIRRORV_PATTERN=1" Shader.frag
CALL :glslc 1 Path_Pattern_Mirror_PS "-DEFFECT_PATH=1" "-DPAINT_PATTERN=1" "-DMIRROR_PATTERN=1" Shader.frag

CALL :glslc 1 Path_AA_Solid_PS "-DEFFECT_PATH_AA=1" "-DPAINT_SOLID=1" Shader.frag
CALL :glslc 1 Path_AA_Linear_PS "-DEFFECT_PATH_AA=1" "-DPAINT_LINEAR=1" Shader.frag
CALL :glslc 1 Path_AA_Radial_PS "-DEFFECT_PATH_AA=1" "-DPAINT_RADIAL=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_Clamp_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" "-DCLAMP_PATTERN=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_Repeat_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" "-DREPEAT_PATTERN=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_MirrorU_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" "-DMIRRORU_PATTERN=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_MirrorV_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" "-DMIRRORV_PATTERN=1" Shader.frag
CALL :glslc 1 Path_AA_Pattern_Mirror_PS "-DEFFECT_PATH_AA=1" "-DPAINT_PATTERN=1" "-DMIRROR_PATTERN=1" Shader.frag

CALL :glslc 2 SDF_Solid_PS "-DEFFECT_SDF=1" "-DPAINT_SOLID=1" Shader.frag
CALL :glslc 2 SDF_Linear_PS "-DEFFECT_SDF=1" "-DPAINT_LINEAR=1" Shader.frag
CALL :glslc 2 SDF_Radial_PS "-DEFFECT_SDF=1" "-DPAINT_RADIAL=1" Shader.frag
CALL :glslc 2 SDF_Pattern_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" Shader.frag
CALL :glslc 2 SDF_Pattern_Clamp_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" "-DCLAMP_PATTERN=1" Shader.frag
CALL :glslc 2 SDF_Pattern_Repeat_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" "-DREPEAT_PATTERN=1" Shader.frag
CALL :glslc 2 SDF_Pattern_MirrorU_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" "-DMIRRORU_PATTERN=1" Shader.frag
CALL :glslc 2 SDF_Pattern_MirrorV_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" "-DMIRRORV_PATTERN=1" Shader.frag
CALL :glslc 2 SDF_Pattern_Mirror_PS "-DEFFECT_SDF=1" "-DPAINT_PATTERN=1" "-DMIRROR_PATTERN=1" Shader.frag

CALL :glslc 1 Opacity_Solid_PS "-DEFFECT_OPACITY=1" "-DPAINT_SOLID=1" Shader.frag
CALL :glslc 1 Opacity_Linear_PS "-DEFFECT_OPACITY=1" "-DPAINT_LINEAR=1" Shader.frag
CALL :glslc 1 Opacity_Radial_PS "-DEFFECT_OPACITY=1" "-DPAINT_RADIAL=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_Clamp_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" "-DCLAMP_PATTERN=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_Repeat_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" "-DREPEAT_PATTERN=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_MirrorU_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" "-DMIRRORU_PATTERN=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_MirrorV_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" "-DMIRRORV_PATTERN=1" Shader.frag
CALL :glslc 1 Opacity_Pattern_Mirror_PS "-DEFFECT_OPACITY=1" "-DPAINT_PATTERN=1" "-DMIRROR_PATTERN=1" Shader.frag

CALL :glslc 1 Upsample_PS "-DEFFECT_UPSAMPLE=1" Shader.frag
CALL :glslc 1 Downsample_PS "-DEFFECT_DOWNSAMPLE=1" Shader.frag

CALL :glslc 1 Shadow_PS "-DEFFECT_SHADOW=1" "-DPAINT_SOLID=1" Shader.frag
CALL :glslc 1 Blur_PS "-DEFFECT_BLUR=1" "-DPAINT_SOLID=1" Shader.frag

:: Compress
Pack Shaders.tar Shaders.z
bin2h.py Shaders.z Shaders >> Shaders.h

del Shaders.z
del Shaders.tar

EXIT /B 0

:: -------------------------------------------------------------------------------------------------------
:: glslc.exe is part of the Vulkan SDK (https://www.lunarg.com/vulkan-sdk)
:: -------------------------------------------------------------------------------------------------------
:glslc
    glslc.exe -O -fauto-bind-uniforms -fcbuffer-binding-base %1 -ftexture-binding-base %1 -o %2.spv %3 %4 %5 %6 %7 %8 %9 > NUL
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    tar.py Shaders.tar %2.spv >> Shaders.h
    del %2.spv
    EXIT /B 0