#version 130
#include "shared-helpers.glsl"
in vec4 pos_wc;
in vec3 view_wc;
in vec3 view_vc;
in vec3 norm_wc;
in vec3 norm_v;
in vec2 tc;
uniform sampler2D diffuse;

void main() {
    float depth = linear_depth(gl_FragCoord.z, NEAR, FAR);
    float dddx = dFdx(depth);
    float dddy = dFdy(depth);
    if (dddx > 0.25/FAR || dddy > 0.25/FAR ) discard;
}
