// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"
#include "cpp_shared_structures.h"
#include "filmic_tonemapper.h"

layout (location = 0) in vec3 outSkyColor;
layout (location = 1) in vec2 outScreenPos;
layout (location = 2) in vec3 outViewDir;

layout (location = 0) out vec4 outFragColor;

//uniformly distributed, normalized rand, [0, 1)
float nrand(in vec2 n)
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

float n4rand_ss(in vec2 n)
{
	float nrnd0 = nrand( n + 0.07*fract( uboPerFrame.params.w ) );
	float nrnd1 = nrand( n + 0.11*fract( uboPerFrame.params.w + 0.573953 ) );
	return 0.23*sqrt(-log(nrnd0+0.00001))*cos(2.0*3.141592*nrnd1)+0.5;
}

void main()
{
	float size2 = uboPerFrame.params.x * uboPerFrame.params.x;
	vec3 lightDir = -normalize(uboPerFrame.sunDir.xyz);
	float distance = 2.0 * (1.0 - dot(normalize(outViewDir), lightDir));
	float sun = exp(-distance/ uboPerFrame.params.y / size2) + step(distance, size2);
	float sun2 = min(sun * sun, 1.0);
	vec3 color = outSkyColor + sun2;
	color = pow(color, vec3(1.0 / 2.2));
	float r = n4rand_ss(outScreenPos);
	color += vec3(r, r, r) / 40.0;
	outFragColor = vec4(color, 1.0);
}