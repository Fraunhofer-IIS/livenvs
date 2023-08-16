#version 130
#include "shared-helpers.glsl"
in vec2 tc;
uniform sampler2D depth_tex;
uniform mat4 inv_mat;
uniform vec3 valid_range_min_max_invalid;
out vec4 out_col;

vec4 to_world(vec4 pos){
    pos = inv_mat*pos;
    pos /= pos.w;
    return pos;
}

void main() {
    float d = texture(depth_tex, tc).r;
    d = clamp(d, 0, 1);
    // if (d < 1e-10) d = 1;
    vec4 pos = to_world(2*vec4(tc, d,1)-1);
    d = -pos.z;

    if (d > valid_range_min_max_invalid.y) d = valid_range_min_max_invalid.z;
    if (d < valid_range_min_max_invalid.x ) d = valid_range_min_max_invalid.z;

    // d = clamp(d, 0, 30);
    // if (d < 0.01 ) d = 30;
    
    // d = clamp(d, 0.01, 22.0);
    out_col = vec4(d);
}
