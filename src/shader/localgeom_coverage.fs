#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "fragment_fusion.glsl"

in vec2 tc;
in vec3 tgt_view_dir_ws;
in vec3 src_view_dir_ws;

uniform sampler2D color_tex;
uniform sampler2D depth_tex;
out float out_col;

void main() {
    float weight = 1;
    weight += 0.1*vignette_weight(tc);
    weight += view_dir_weight(tgt_view_dir_ws, src_view_dir_ws);
    weight += max(0.1, depth_weight(linear_depth(gl_FragCoord.z, NEAR, FAR))); 
    out_col = max(0.01, weight);
}
