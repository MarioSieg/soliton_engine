$input a_position, a_normal, a_texcoord0, a_tangent, a_bitangent
$output v_position, v_normal, v_texcoord0, v_tangent, v_bitangent

#include "common.shh"
#include "uniforms.shh"

uniform mat3 u_normalMtx;

void main() {
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_position = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_normal = mul(u_normalMtx, a_normal).xyz;
    v_tangent = mul(u_model[0], vec4(a_tangent, 0.0)).xyz;
    v_texcoord0 = a_texcoord0;
    v_bitangent = a_bitangent;
}
