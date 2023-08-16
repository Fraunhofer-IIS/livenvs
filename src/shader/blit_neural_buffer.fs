#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "shared-helpers.glsl"
in vec2 tc;
in vec3 tgt_view_dir_ws;

uniform int WIDTH;
uniform int HEIGHT;

out vec3 tgt_view;
out vec3 src_view;
out float nonlin_depth;

out vec4 out_feat[FEAT_SIZE];
// out vec4 out_feat0;
// out vec4 out_feat1;
// out vec4 out_feat2;
// out vec4 out_feat3;

vec4 var_vec4(in vec4 feat, in vec4 sqr_feat){
    vec4 var = feat;
    var *= var;
    var = sqrt(sqr_feat - var);
    return var;
}

Feature var_feat(in Feature feat, in Feature sqr_feat){
    Feature var;
    for(int i = 0; i< FEAT_SIZE; i++)
        var.feat[i] = var_vec4(feat.feat[i], sqr_feat.feat[i]);
    return var;
}

void main() {
    // uint index = int(round(tc.x*(WIDTH)-0.5)) + int(round(tc.y*(HEIGHT)-0.5)) * WIDTH;    
    // uint index = int(gl_FragCoord.x) + int(gl_FragCoord.y) * WIDTH;

    ivec2 fragCoord = ivec2(round(max(vec2(0),tc * vec2(WIDTH, HEIGHT)-0.5)));
    uint index = int(fragCoord.x) + int(fragCoord.y) * WIDTH;
    
    src_view = fragmentdata[index].src_view_dir;
    tgt_view = tgt_view_dir_ws;
    nonlin_depth = nonlinear_depth(fragmentdata[index].depth, NEAR, FAR);
    // gl_FragDepth = fragmentdata[index].depth;
    
    // out_feat0 = fragmentdata[index].feat.feat[0];
    // out_feat1 = fragmentdata[index].feat.feat[1];
    // out_feat2 = fragmentdata[index].feat.feat[2];
    // out_feat3 = fragmentdata[index].feat.feat[3];

    for(int i = 0; i < FEAT_SIZE; i++){
        out_feat[i] = fragmentdata[index].feat.feat[i];
    }    

    // out_col = vec4(counter[index]);
}
