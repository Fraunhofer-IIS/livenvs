#version 130
#include "shared-helpers.glsl"
in vec2 tc;
uniform sampler2D depth_tex;
uniform int WIDTH;
uniform int HEIGHT;
out vec4 out_col;

float t = 0.0005;
float mask_val(float d){
    return 1-clamp((d -(1-t))*(1/t),0,1);
}

void main() {
    float lvl= 2;

    float mask = 0.0;
    for (int i = -1; i <= 1; ++i){
        for (int j = -1; j <= 1; ++j){
            vec2 tcx = tc+vec2(i*pow(lvl,2)/WIDTH, j*pow(lvl,2)/HEIGHT);
            float d  = textureLod(depth_tex, tcx, lvl).r;
            mask+=mask_val(d);
        }
    }

    out_col = vec4(mask/9);
}
