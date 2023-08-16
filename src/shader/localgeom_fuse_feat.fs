#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "fragment_fusion.glsl"
#extension  GL_ARB_gpu_shader_int64 : enable
#extension GL_NV_shader_atomic_float : enable
#extension GL_NV_shader_atomic_int64 : enable

in vec2 tc;
in vec3 debug;
in vec3 tgt_view_dir_ws;
in vec3 src_view_dir_ws;

uniform int WIDTH;
uniform int HEIGHT;
uniform float beta_tgt;
uniform float beta_src;
uniform ivec4 use_weights;

uniform sampler2D view_tex;
uniform sampler2D depth_tex;
uniform sampler2DArray feat_tex;

out vec3 tgt_view;
out vec3 src_view;

out vec4 out_feat0;
out vec4 out_feat1;
out vec4 out_feat2;
out vec4 out_feat3;

void main() {

    uint index = int(gl_FragCoord.x) + int(gl_FragCoord.y) * WIDTH;

    float lin_src_depth = max(0.0001,linear_depth(texture(depth_tex, tc).r, NEAR, FAR));

    if(any( lessThan(tc, vec2(0.01))) || any(greaterThan(tc, vec2(0.99)))) {discard; return;}

    FragmentData this_frag;
    this_frag.src_view_dir = src_view_dir_ws;
    this_frag.depth = linear_depth(gl_FragCoord.z, NEAR, FAR);
    this_frag.tdis = calc_tdis(this_frag.depth, beta_tgt, lin_src_depth, beta_src);
        this_frag.weight = 1;
    if (use_weights.x > 0)
        this_frag.weight *= vignette_weight(tc);
    if (use_weights.y > 0)
        this_frag.weight *= view_dir_weight(tgt_view_dir_ws, src_view_dir_ws);
    if (use_weights.z > 0)
        this_frag.weight *= depth_weight(lin_src_depth); 
        // XXX: ensured to be non-negative in fragment_fusion code! // TODO XXX beta
    // this_frag.weight = debug.x*dot(tgt_view_dir_ws, src_view_dir_ws)/lin_src_depth; // TODO: add div by lin depth
    this_frag.weight  = pow(this_frag.weight, float(use_weights.w));

    this_frag.padding = 42.0f;
    this_frag.feat = from_tex_array(feat_tex, tc);

    fragment_fusion(this_frag, index);
    
}
