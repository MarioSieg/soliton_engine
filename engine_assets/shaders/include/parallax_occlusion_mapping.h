#ifndef PARALLAX_OCCLUSION_MAPPING_H
#define PARALLAX_OCCLUSION_MAPPING_H

const float NUM_LAYERS = 48.0;
const float HEIGHT_SCALE = 0.3;

vec2 parallaxOcclusionMapping(const vec2 uv, const vec3 viewDir) {
    float layerDepth = 1.0 / NUM_LAYERS;
    float currLayerDepth = 0.0;
    vec2 deltaUV = viewDir.xy * HEIGHT_SCALE / (viewDir.z * NUM_LAYERS);
    vec2 currUV = uv;
    float height = 1.0 - textureLod(sHeightMap, currUV, 0.0).a;
    for (int i = 0; i < NUM_LAYERS; ++i) {
        currLayerDepth += layerDepth;
        currUV -= deltaUV;
        height = 1.0 - textureLod(sHeightMap, currUV, 0.0).a;
        if (height < currLayerDepth) {
            break;
        }
    }
    vec2 prevUV = currUV + deltaUV;
    float nextDepth = height - currLayerDepth;
    float prevDepth = 1.0 - textureLod(sHeightMap, prevUV, 0.0).a - currLayerDepth + layerDepth;
    return mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

#endif
