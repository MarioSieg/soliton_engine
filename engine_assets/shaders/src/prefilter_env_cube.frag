// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#version 450

#include "pbr_common.h"

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform samplerCube samplerEnv;

layout(push_constant) uniform PushConsts {
	layout (offset = 64) float roughness;
	layout (offset = 68) uint numSamples;
} consts;

// Normal Distribution function
float normalDistributionGGX(const float dotNH, const float roughness) {
	const float alpha = roughness * roughness;
	const float alpha2 = alpha * alpha;
	const float denom = dotNH*dotNH*(alpha2 - 1.0) + 1.0;
	return (alpha2)/(kPI*denom*denom);
}

// Filtering based on https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
vec3 prefilterEnvMap(vec3 R, float roughness) {
	vec3 N = R;
	vec3 V = R;
	vec3 color = vec3(0.0);
	float totalWeight = 0.0;
	float envMapDim = float(textureSize(samplerEnv, 0).s);
	for(uint i = 0u; i < consts.numSamples; i++) {
		vec2 Xi = hammersley2D(i, consts.numSamples);
		vec3 H = importanceSample_GGX(Xi, roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;
		float dotNL = clamp(dot(N, L), 0.0, 1.0);
		if(dotNL > 0.0) {
			float dotNH = clamp(dot(N, H), 0.0, 1.0);
			float dotVH = clamp(dot(V, H), 0.0, 1.0);

			float pdf = normalDistributionGGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001; // Probability Distribution Function
			float omegaS = 1.0 / (float(consts.numSamples) * pdf); // Slid angle of current smple
			float omegaP = 4.0 * kPI / (6.0 * envMapDim * envMapDim); // Solid angle of 1 pixel across all cube faces
			float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f); // Biased (+1.0) mip level for better result
			color += textureLod(samplerEnv, L, mipLevel).rgb * dotNL;
			totalWeight += dotNL;

		}
	}
	return (color / totalWeight);
}

void main() {
	vec3 N = normalize(inPos);
	outColor = vec4(prefilterEnvMap(N, consts.roughness), 1.0);
}
