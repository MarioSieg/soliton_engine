// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"
#include "cpp_shared_structures.h"
#include "filmic_tonemapper.h"
#include "lens_flare.h"

layout (location = 0) in vec3 outSkyColor;
layout (location = 1) in vec2 outScreenPos;
layout (location = 2) in vec3 outViewDir;

layout (location = 0) out vec4 outFragColor;

vec2 computeSunScreenCoords(vec3 sunWorldPos)
{
	vec4 sunClipPos = uboPerFrame.viewProj * vec4(sunWorldPos, 1.0);
	vec3 sunNDC = sunClipPos.xyz / sunClipPos.w;
	vec2 sunScreenCoords = sunNDC.xy * 0.5 + 0.5;
	return sunScreenCoords;
}

void main()
{
	vec3 color = outSkyColor;
	vec2 texcoord = (outScreenPos * 0.5) + 0.5;
	vec2 sunCoord = computeSunScreenCoords(-uboPerFrame.sunDir.xyz * 65535.0); // Multiply by a large number to simulate sun's position at infinity
	vec3 ghostColor;
	vec3 lensFlareColor = lensFlare(texcoord, sunCoord, ghostColor);
	color += (ghostColor + lensFlareColor*0.5) * uboPerFrame.sky.sun_luminance.rgb * 0.1;
	color = postToneMap(color);
	outFragColor = vec4(color, 1.0);
}