// Copyright (c) 2022-2024 Mario "Neo" Sieg. All Rights Reserved.

#ifndef FILMIC_TONEMAPPER_H
#define FILMIC_TONEMAPPER_H

#include "aces.h"

vec3 filmToneMap(
    vec3 colorAP1,
    float filmSlope,
    float filmToe,
    float filmShoulder,
    float filmBlackClip,
    float filmWhiteClip,
    float filmPreDesaturate,
    float filmPostDesaturate,
    float filmRedModifier,
    float filmGlowScale) {
    vec3 colorAP0 = colorAP1 *  AP1_2_XYZ_MAT * XYZ_2_AP0_MAT;

    // "Glow" module constants
    const float RRT_GLOW_GAIN = 0.05;
    const float RRT_GLOW_MID = 0.08;
    float saturation = rgb_2_saturation(colorAP0);
    float ycIn = rgb_2_yc(colorAP0, 1.75);
    float s = sigmoid_shaper((saturation - 0.4) / 0.2);
    float addedGlow = 1 + glow_fwd(ycIn, RRT_GLOW_GAIN * s, RRT_GLOW_MID) * filmGlowScale;
    colorAP0 *= addedGlow;

    // --- Red modifier --- //
    const float RRT_RED_SCALE = 0.82;
    const float RRT_RED_PIVOT = 0.03;
    const float RRT_RED_HUE = 0;
    const float RRT_RED_WIDTH = 135;
    float hue = rgb_2_hue(colorAP0);
    float centeredHue = center_hue(hue, RRT_RED_HUE);
    float hueWeight = smoothstep(0, 1, 1 - abs(2 * centeredHue / RRT_RED_WIDTH));
    hueWeight = hueWeight * hueWeight;
    colorAP0.r += mix(0.0, hueWeight * saturation * (RRT_RED_PIVOT - colorAP0.r) * (1.0 - RRT_RED_SCALE), filmRedModifier);

    // Use ACEScg primaries as working space
    vec3 workingColor = colorAP0 * AP0_2_AP1_MAT;
    workingColor = max(vec3(0), workingColor);

    // Pre desaturate
    workingColor = mix(vec3(dot(workingColor, AP1_RGB2Y)), workingColor, filmPreDesaturate);

    const float toeScale	    = 1.0 + filmBlackClip - filmToe;
    const float shoulderScale	= 1.0 + filmWhiteClip - filmShoulder;

    const float inMatch = 0.18;
    const float outMatch = 0.18;

    float toeMatch;
    if(filmToe > 0.8) {
        // 0.18 will be on straight segment
        toeMatch = (1 - filmToe  - outMatch) / filmSlope + log10(inMatch);
    }
    else {
        // 0.18 will be on toe segment

        // Solve for toeMatch such that input of inMatch gives output of outMatch.
        const float bt = (outMatch + filmBlackClip) / toeScale - 1;
        toeMatch = log10(inMatch) - 0.5 * log((1+bt) / (1-bt)) * (toeScale / filmSlope);
    }

    float straightMatch = (1.0 - filmToe) / filmSlope - toeMatch;
    float shoulderMatch = filmShoulder / filmSlope - straightMatch;

    vec3 logColor = log10(workingColor);
    vec3 straightColor = filmSlope * (logColor + straightMatch);

    vec3 toeColor		= (    - filmBlackClip ) + (2.0 *      toeScale) / (1.0 + exp( (-2.0 * filmSlope /      toeScale) * (logColor -      toeMatch)));
    vec3 shoulderColor	= (1.0 + filmWhiteClip ) - (2.0 * shoulderScale) / (1.0 + exp( ( 2.0 * filmSlope / shoulderScale) * (logColor - shoulderMatch)));

    toeColor.x		= logColor.x <      toeMatch ?      toeColor.x : straightColor.x;
    toeColor.y		= logColor.y <      toeMatch ?      toeColor.y : straightColor.y;
    toeColor.z		= logColor.z <      toeMatch ?      toeColor.z : straightColor.z;

    shoulderColor.x	= logColor.x > shoulderMatch ? shoulderColor.x : straightColor.x;
    shoulderColor.y	= logColor.y > shoulderMatch ? shoulderColor.y : straightColor.y;
    shoulderColor.z	= logColor.z > shoulderMatch ? shoulderColor.z : straightColor.z;

    vec3 t = saturate((logColor - toeMatch) / (shoulderMatch - toeMatch));
    t = shoulderMatch < toeMatch ? 1 - t : t;
    t = (3.0 - 2.0 * t) * t * t;

    vec3 toneColor = mix(toeColor, shoulderColor, t);

    // Post desaturate
    toneColor = mix(vec3(dot(vec3(toneColor), AP1_RGB2Y)), toneColor, filmPostDesaturate);

    // Returning positive AP1 values
    return max(vec3(0), toneColor);
}

const float expandGamutFactor = 1.0f;

// Gamma curve encode to srgb.
vec3 encodeSRGB(vec3 linearRGB)
{
    // Most PC Monitor is 2.2 Gamma, maybe this function is enough.
    // return pow(linearRGB, vec3(1.0 / 2.2));

    // TV encode Rec709 encode.
    vec3 a = 12.92 * linearRGB;
    vec3 b = 1.055 * pow(linearRGB, vec3(1.0 / 2.4)) - 0.055;
    vec3 c = step(vec3(0.0031308), linearRGB);
    return mix(a, b, c);
}

vec3 postToneMap(vec3 hdrColor)
{
    vec3 colorAP1 = hdrColor * sRGB_2_AP1_MAT;
    if (expandGamutFactor > 0.0f) {
        // NOTE: Expand to wide gamut.
        //       We render in linear srgb color space, and tonemapper in acescg.
        //       acescg gamut larger than srgb, we use this expand matrix to disguise rendering in acescg color space.
        float lumaAP1 = dot(colorAP1, AP1_RGB2Y);
        vec3 chromaAP1 = colorAP1 / lumaAP1;
        float chromaDistSqr = dot(chromaAP1 - 1.0, chromaAP1 - 1.0);
        float expandAmount =
            (1.0 - exp2(-4.0 * chromaDistSqr)) *
            (1.0 - exp2(-4.0 * expandGamutFactor * lumaAP1 * lumaAP1));

        const mat3 Wide_2_XYZ_MAT = mat3(
            vec3( 0.5441691,  0.2395926,  0.1666943),
            vec3( 0.2394656,  0.7021530,  0.0583814),
            vec3(-0.0023439,  0.0361834,  1.0552183)
        );
        const mat3 Wide_2_AP1 = Wide_2_XYZ_MAT * XYZ_2_AP1_MAT;
        const mat3 expandMat = AP1_2_sRGB_MAT * Wide_2_AP1;
        vec3 colorExpand = colorAP1 * expandMat;
        colorAP1 = mix(colorAP1, colorExpand, expandAmount);
    }

    vec3 toneAp1 = filmToneMap( // Apply filmic tonemapper.
        colorAP1,
        0.91,
        0.55,
        0.26,
        0.00,
        0.04,
        0.96,
        0.93,
        1.00,
        1.00
    );

    // Convert to srgb.
    vec3 srgbColor = toneAp1 * AP1_2_sRGB_MAT;

    // OETF part.
    // Encode to fit monitor gamma curve, current default use Rec.709 gamma encode.
    vec3 encodeColor = encodeSRGB(srgbColor);
    return encodeColor;
}

#endif
