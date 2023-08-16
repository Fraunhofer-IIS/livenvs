#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "shared-helpers.glsl"
in vec2 tc;
uniform int WIDTH;
uniform int HEIGHT;
out vec4 out_col;
out vec4 out_col1;
out vec4 out_col2;
out vec4 out_col3;
out vec4 out_nonlin_depth;
void main() {
    ivec2 fragCoord = ivec2(round(max(vec2(0),tc * vec2(WIDTH, HEIGHT)-0.5)));
    int index = fragCoord.x + fragCoord.y * WIDTH;
    // uint index = int(gl_FragCoord.x) + int(gl_FragCoord.y) * WIDTH;
    out_col = fragmentdata[index].feat.feat[0];
    out_col1  = vec4(1,1,0,1);
    out_col2 = vec4(0,1,0,1);
    out_col3 = vec4(1,0,1,1);
    out_nonlin_depth = vec4(nonlinear_depth(fragmentdata[index].depth, NEAR, FAR));
    // out_col = vec4(counter[index]);
}
