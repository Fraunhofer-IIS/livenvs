#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "shared-helpers.glsl"
in vec2 fs_tc;
uniform int WIDTH;
uniform int HEIGHT;
out vec4 out_col;
void main() {
    ivec2 fragCoord = ivec2(round(max(vec2(0),fs_tc * vec2(WIDTH, HEIGHT)-0.5)));
    int index = fragCoord.x + fragCoord.y * WIDTH;
    // uint index = int(gl_FragCoord.x) + int(gl_FragCoord.y) * WIDTH;
    if (gl_Layer == 0){
        out_col = fragmentdata[index].feat.feat[0];
    } else if (gl_Layer == 1) {
        out_col  = vec4(1,1,0,1);        
    } else if (gl_Layer == 2) {
        out_col = vec4(0,1,0,1);
    } else if (gl_Layer == 3) {
        out_col = vec4(1,0,1,1);        
    } else if (gl_Layer == 4) {
        out_col = vec4(nonlinear_depth(fragmentdata[index].depth, NEAR, FAR));
    }
}
