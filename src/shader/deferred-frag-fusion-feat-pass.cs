
#version 450
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "shared-helpers.glsl"


uniform int WIDTH;
uniform int HEIGHT;

uniform layout (location=0) sampler2D tgt_depth_texx;
uniform layout (location=1) sampler2D src_depth_tex;
uniform layout (location=2) sampler2DArray src_feat;
uniform mat4 inv_tgt_proj_view;
uniform mat4 inv_tgt_proj;
uniform mat4 src_proj_view;
uniform mat4 inv_src_proj;

uniform vec3 tgt_cam_wpos;
uniform vec3 src_cam_wpos;
uniform ivec4 use_weights;

uniform float background_t;
uniform bool use_background;


layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    if(storePos.x >= WIDTH || storePos.y >= HEIGHT)
        return;
    int index = int(storePos.x) + int(storePos.y) * WIDTH;

    // if (fragmentdata[index].def_weight_sum < 0){
    //     fragmentdata[index].feat = debug_feat(vec4(1)); 
    //     fragmentdata[index].def_weight_sum = 0; 
    // }
    bool background = false;

    vec2 tgt_tc = (vec2(storePos)+vec2(0.5))/vec2(WIDTH, HEIGHT);
    float tgt_depth_buf = nonlinear_depth(fragmentdata[index].depth, NEAR, FAR);
    float tgt_depth = clamp(texture(tgt_depth_texx, tgt_tc).r,0.0,1.0);
    if(tgt_depth == 1){
        background = true;
    }
    
    vec4 pos = 2*vec4(tgt_tc, tgt_depth,1)-1;
    pos = inv_tgt_proj*pos;
    pos /= pos.w;
    float vspace_tgt_depth_linear = -pos.z;
    if(vspace_tgt_depth_linear > background_t){ // 
        background = true;
    }

    vec4 tgt_pos = 2*vec4(tgt_tc, tgt_depth, 1)-1;
    vec4 wpos = inv_tgt_proj_view * tgt_pos;
    wpos /= wpos.w;
    vec4 src_pos = src_proj_view * wpos;
    src_pos /= src_pos.w;
    vec3 src_tc = .5*src_pos.xyz+.5;

    float border = 0.01;
    // if(!use_background){
        if(any( lessThan(src_tc, vec3(border,border,0))) || any(greaterThan(src_tc, vec3(1-border, 1-border,1)))) {
            // fragmentdata[index].feat = debug_feat(vec4(0.5,0.5,0.5,1)); 
            return;
        }
    // }

    vec3 tgt_view_dir_ws = normalize(tgt_cam_wpos - wpos.xyz);
    vec3 src_view_dir_ws = normalize(src_cam_wpos - wpos.xyz);

    float lin_src_depth = linear_depth(src_tc.z, NEAR, FAR);
    float actual_nonlin_src_depth = clamp(textureLod(src_depth_tex, src_tc.xy,0).r,0,1);
    float actual_lin_src_depth = linear_depth(actual_nonlin_src_depth, NEAR, FAR);

    float actual_nonlin_src_depth_lod = clamp(textureLod(src_depth_tex, src_tc.xy,3).r,0,1);

   

    pos = 2*vec4(src_tc.xy, actual_nonlin_src_depth,1)-1;
    pos = inv_src_proj*pos;
    pos /= pos.w;
    float vspace_actual_lin_src_depth = -pos.z;
    bool src_background = vspace_actual_lin_src_depth > background_t;// || vspace_actual_lin_src_depth < 0.01;

    // according to fragment_fusion this should be 
    //  float tdis = fragmentdata[index].tdis + calc_tdis(float linear_depth_tgt, float tgt_scale, float linear_depth_src, float src_scale);
    // we approximate by simply doubling the existent tdis
    float tgt_tdis = fragmentdata[index].tdis;
    float tdis = 3*tgt_tdis;
    float dis = abs(actual_lin_src_depth-lin_src_depth); 


    if(use_background && background && src_background){
        float lod_dist = 5*abs(linear_depth(actual_nonlin_src_depth_lod, NEAR, FAR)-actual_lin_src_depth);
        if (lod_dist > 0.8) {
            // fragmentdata[index].feat.feat0 =  vec4(0,1,0,1);
            return;
        }

        float weight = 1;
        // weight *= w_dis;
        if (use_weights.x > 0)
            weight *= vignette_weight(src_tc.xy);
        if (use_weights.y > 0)
            weight *= view_dir_weight(tgt_view_dir_ws, src_view_dir_ws);
        // if (use_weights.z > 0)
        //     weight *= depth_weight(lin_src_depth); 
        weight = pow(weight, float(use_weights.w));

        weight = max(MIN_FUSE_WEIGHT, weight);
        weight = min(1,weight);
        // if(abs(tgt_depth - tgt_depth_buf) > 0.00001) {
        //     weight *= 0.00001;
        // }

        Feature_Arr feat = from_tex_array_arr(src_feat,  src_tc.xy);

        float alpha = fragmentdata[index].def_weight_sum / (fragmentdata[index].def_weight_sum + weight);
        fragmentdata[index].feat = blend_feat_arr(fragmentdata[index].feat, feat, alpha);

        fragmentdata[index].def_weight_sum = fragmentdata[index].def_weight_sum + weight;

        return;
    } 
    else if (background || src_background){
        // fragmentdata[index].feat.feat0 =  vec4(1,0,0,1);
        return;
    }


    // // new?
    // tdis = 0.015/FAR;
    float w_dis = clamp(1- clamp(pow(dis/tdis,2), 0,1),0,1);
    // {
    // before
    if( dis < tdis 
    // //     // || abs(tgt_depth - tgt_depth_buf) > 0.00001
        ){
        // passed depth test or depth was hole filled
        float weight = 1;
        weight *= w_dis;
        if (use_weights.x > 0)
            weight *= vignette_weight(src_tc.xy);
        if (use_weights.y > 0)
            weight *= view_dir_weight(tgt_view_dir_ws, src_view_dir_ws);
        if (use_weights.z > 0)
            weight *= depth_weight(lin_src_depth); 
        weight = pow(weight, float(use_weights.w));

        weight = max(MIN_FUSE_WEIGHT, weight);
        weight = min(1,weight);
        // if(abs(tgt_depth - tgt_depth_buf) > 0.00001) {
        //     weight *= 0.00001;
        // }

        Feature_Arr feat = from_tex_array_arr(src_feat,  src_tc.xy); 

        float alpha = fragmentdata[index].def_weight_sum / (fragmentdata[index].def_weight_sum + weight);
        fragmentdata[index].feat = blend_feat_arr(fragmentdata[index].feat, feat, alpha);

        fragmentdata[index].def_weight_sum = fragmentdata[index].def_weight_sum + weight;
    }
    
}
