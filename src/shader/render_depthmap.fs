#version 130
#include "shared-helpers.glsl"
in vec2 tc;
uniform sampler2D depth_tex;
uniform int WIDTH;
uniform int HEIGHT;
uniform mat4 inv_proj_view;
out vec4 out_col;

vec4 to_world(vec4 pos){
    pos = inv_proj_view*pos;
    pos /= pos.w;
    return pos;
}

void main() {
    vec2 tcx = tc+vec2(1.0/WIDTH, 0);
    vec2 tcy = tc+vec2(0, 1.0/HEIGHT);

    float d = texture(depth_tex, tc).r;
    float dx = texture(depth_tex, tcx).r;
    float dy = texture(depth_tex, tcy).r;

    vec4 pos = to_world(2*vec4(tc, d,1)-1);
    vec4 posx = to_world(2*vec4(tcx, dx,1)-1);
    vec4 posy = to_world(2*vec4(tcy, dy,1)-1);


    vec3 horizontal = (pos.xyz-posx.xyz);
    vec3 vertical = (pos.xyz-posy.xyz);

    if (length(horizontal) > 0.1 || length(horizontal) > 0.1){
        out_col = vec4(1.0);
        return;
    }
    vec3 norm = normalize(cross(horizontal, vertical));
    out_col = vec4(130, 106, 90, 0)/255 + vec4(1.0)*max(0,dot(norm, normalize(vec3(1,-1,-0.2))));
}
