$input v_position, v_normal, v_texcoord0, v_tangent, v_bitangent

#include "common.shh"
#include "uniforms.shh"
#include "pbr.shh"

#define MAX_REFLECTION_LOD 8.0

SAMPLER2D(s_texLUT, 0);
SAMPLERCUBE(s_texCube, 1);
SAMPLERCUBE(s_texCubeIrr, 2);
SAMPLER2D(s_texAlbedo, 3);
SAMPLER2D(s_texNormal, 4);
SAMPLER2D(s_texMetallic, 5);
SAMPLER2D(s_texRoughness, 6);
SAMPLER2D(s_texAO, 7);

void main() {
	// Sample maps.
	vec3 albedo = texture2D(s_texAlbedo, v_texcoord0).rgb;
	vec3 normal = normalize(texture2D(s_texNormal, v_texcoord0).rgb * 2.0 - 1.0);
	//float metallic = texture2D(s_texMetallic, v_texcoord0).r;
	//float roughness = texture2D(s_texRoughness, v_texcoord0).r;
    float metallic = u_reflectivity;
    float roughness = u_glossiness;
	float ao = texture2D(s_texAO, v_texcoord0).r;

	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);
	vec3 N = normalize(mul(tbn, normal));
	vec3 V = normalize(u_camPos - v_position);
    vec3 R = reflect(-V, N);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3_splat(0.04); 
    F0 = mix(F0, albedo, metallic);

	// reflectance equation
    vec3 Lo = vec3_splat(0.0);
	{
		// calculate per-light radiance
        vec3 L = normalize(vec3(0.0, 1.5, 0.0) - v_position);
        vec3 H = normalize(V + L);
        float distance = length(vec3(0.0, 1.5, 0.0) - v_position);
        float attenuation = 0.0;
        vec3 radiance = vec3_splat(1.0) * attenuation;

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);   
        float G = geometrySmith(N, V, L, roughness);    
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3_splat(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}
	
	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = textureCube(s_texCubeIrr, N).rgb;
    vec3 diffuse = irradiance * albedo;

	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    vec3 prefilteredColor = textureCubeLod(s_texCube, R, roughness * MAX_REFLECTION_LOD).rgb; 
    vec2 brdf  = texture2D(s_texLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;
    gl_FragColor = vec4(color , 1.0);
}
