$input a_position, a_normal
$output v_view, v_normal

#include "common.shh"
#include "uniforms.shh"

void main() {
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_view = u_camPos - mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_normal = mul(u_model[0], vec4(a_normal, 0.0) ).xyz;
}
