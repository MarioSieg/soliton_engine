// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "shader_common.h"
#include "cpp_shared_structures.h"

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outSkyColor;
layout (location = 1) out vec2 outScreenPos;
layout (location = 2) out vec3 outViewDir;

vec3 Perez(vec3 A,vec3 B,vec3 C,vec3 D, vec3 E,float costeta, float cosgamma) {
	float _1_costeta = 1.0 / costeta;
	float cos2gamma = cosgamma * cosgamma;
	float gamma = acos(cosgamma);
	vec3 f = (vec3(1.0) + A * exp(B * _1_costeta)) * (vec3(1.0) + C * exp(D * gamma) + E * cos2gamma);
	return f;
}

out gl_PerVertex {
	vec4 gl_Position;
};

vec3 convertXYZ2RGB(vec3 _xyz)  {
	vec3 rgb;
	rgb.x = dot(vec3( 3.2404542, -1.5371385, -0.4985314), _xyz);
	rgb.y = dot(vec3(-0.9692660,  1.8760108,  0.0415560), _xyz);
	rgb.z = dot(vec3( 0.0556434, -0.2040259,  1.0572252), _xyz);
	return rgb;
}

void main() {
	outScreenPos = inPos.xy;
	mat4 invViewProj = uboPerFrame.viewProjInverse;
	vec4 rayStart = invViewProj * vec4(vec3(inPos.xy, -1.0), 1.0);
	vec4 rayEnd = invViewProj * vec4(vec3(inPos.xy, 1.0), 1.0);
	rayStart = rayStart / rayStart.w;
	rayEnd = rayEnd / rayEnd.w;
	outViewDir = normalize(rayEnd.xyz - rayStart.xyz);
	outViewDir.y = abs(outViewDir.y);
	gl_Position = vec4(inPos.xy, 1.0, 1.0);
	vec3 lightDir = -normalize(uboPerFrame.sunDir.xyz);
	vec3 skyDir = vec3(0.0, 1.0, 0.0);
	vec3 A = uboPerFrame.perez1.xyz;
	vec3 B = uboPerFrame.perez2.xyz;
	vec3 C = uboPerFrame.perez3.xyz;
	vec3 D = uboPerFrame.perez4.xyz;
	vec3 E = uboPerFrame.perez5.xyz;
	float costeta = max(dot(outViewDir, skyDir), 0.001);
	float cosgamma = clamp(dot(outViewDir, lightDir), -0.9999, 0.9999);
	float cosgammas = dot(skyDir, lightDir);
	vec3 P = Perez(A,B,C,D,E, costeta, cosgamma);
	vec3 P0 = Perez(A,B,C,D,E, 1.0, cosgammas);
	vec3 lum = uboPerFrame.sky_luminance_xyz.xyz;
	vec3 skyColorxyY = vec3(
		lum.x / (lum.x+lum.y + lum.z)
		, lum.y / (lum.x+lum.y + lum.z)
		, lum.y
	);
	vec3 Yp = skyColorxyY * P / P0;
	vec3 skyColorXYZ = vec3(Yp.x * Yp.z / Yp.y,Yp.z, (1.0 - Yp.x- Yp.y)*Yp.z/Yp.y);
	outSkyColor = convertXYZ2RGB(skyColorXYZ * uboPerFrame.params.z);
}
