@echo off

:: Stereo rendering can't be validated
echo Shader.140.stereo.vert
shader2h.py Shader.140.stereo.vert Shader_140_stereo_vert > Shader.140.stereo.vert.h

echo Shader.300es.stereo.vert
shader2h.py Shader.300es.stereo.vert Shader_300es_stereo_vert > Shader.300es.stereo.vert.h

CALL :validate_vertex Shader.140.vert Shader_140_vert
CALL :validate_vertex Shader.100es.vert Shader_100es_vert
CALL :validate_vertex Shader.300es.vert Shader_300es_vert

CALL :validate_fragment Shader.140.frag Shader_140_frag
CALL :validate_fragment Shader.100es.frag Shader_100es_frag
CALL :validate_fragment Shader.300es.frag Shader_300es_frag

goto :eof

:: -------------------------------------------------------------------------------------------------------
:validate_vertex
    echo %1

    CALL :validate %1 || goto :eof
    CALL :validate -DHAS_COLOR %1 || goto :eof
    CALL :validate -DHAS_UV0 %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_RECT %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_RECT %1 -DHAS_TILE || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_COVERAGE %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_COVERAGE %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_COVERAGE -DHAS_RECT %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_COVERAGE -DHAS_RECT -DHAS_TILE %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 -DSDF %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DSDF %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DHAS_RECT -DSDF  %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DHAS_RECT -DHAS_TILE -DSDF  %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DHAS_RECT %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DHAS_RECT -DHAS_TILE %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV0 -DHAS_UV1 %1 || goto :eof
    CALL :validate -DHAS_UV0 -DHAS_UV1 -DDOWNSAMPLE %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 -DHAS_RECT %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV0 -DHAS_RECT -DHAS_IMAGE_POSITION %1 || goto :eof

    :: sRGB variants
    CALL :validate -DHAS_COLOR -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_COVERAGE -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 -DSDF -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV0 -DHAS_UV1 -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV1 -DHAS_RECT -DSRGB %1 || goto :eof
    CALL :validate -DHAS_COLOR -DHAS_UV0 -DHAS_RECT -DHAS_IMAGE_POSITION -DSRGB %1 || goto :eof

    shader2h.py %1 %2 > %1.h

    goto :eof

:: -------------------------------------------------------------------------------------------------------
:validate_fragment
    echo %1

    CALL :validate -DEFFECT_RGBA %1 || goto :eof
    CALL :validate -DEFFECT_MASK %1 || goto :eof
    CALL :validate -DEFFECT_CLEAR %1 || goto :eof

    CALL :validate -DEFFECT_PATH -DPAINT_SOLID %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_LINEAR %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_RADIAL %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN -DCLAMP_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN -DREPEAT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN -DMIRRORU_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN -DMIRRORV_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH -DPAINT_PATTERN -DMIRROR_PATTERN %1 || goto :eof

    CALL :validate -DEFFECT_PATH_AA -DPAINT_SOLID %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_LINEAR %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_RADIAL %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN -DCLAMP_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN -DREPEAT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN -DMIRRORU_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN -DMIRRORV_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_PATH_AA -DPAINT_PATTERN -DMIRROR_PATTERN %1 || goto :eof

    CALL :validate -DEFFECT_SDF -DPAINT_SOLID %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_LINEAR %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_RADIAL %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN -DCLAMP_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN -DREPEAT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN -DMIRRORU_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN -DMIRRORV_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_SDF -DPAINT_PATTERN -DMIRROR_PATTERN %1 || goto :eof

    CALL :validate -DEFFECT_OPACITY -DPAINT_SOLID %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_LINEAR %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_RADIAL %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN -DCLAMP_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN -DREPEAT_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN -DMIRRORU_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN -DMIRRORV_PATTERN %1 || goto :eof
    CALL :validate -DEFFECT_OPACITY -DPAINT_PATTERN -DMIRROR_PATTERN %1 || goto :eof

    CALL :validate -DEFFECT_UPSAMPLE %1 || goto :eof
    CALL :validate -DEFFECT_DOWNSAMPLE %1 || goto :eof

    CALL :validate -DEFFECT_SHADOW -DPAINT_SOLID %1 || goto :eof
    CALL :validate -DEFFECT_BLUR -DPAINT_SOLID %1 || goto :eof

    shader2h.py %1 %2 > %1.h

    goto :eof

:: -------------------------------------------------------------------------------------------------------
:validate
    glslangValidator.exe --quiet -l %*
    if %errorlevel% neq 0 EXIT /B %errorlevel%
    goto :eof