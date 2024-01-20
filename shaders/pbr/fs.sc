$input v_texcoord0

#include "common.shh"

SAMPLER2D(s_sampler, 0);

void main() {
    vec4 tex = texture2D(s_sampler, v_texcoord0);
    tex.a = 1.0;
	gl_FragColor = tex;
}
