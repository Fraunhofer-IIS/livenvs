
#version 450
#include "shared-helpers.glsl"

uniform int WIDTH;
uniform int HEIGHT;

// uniform mat4 from_0_to_merged_mat; 0 is the current view
uniform mat4 from_merged_to_1_mat;
uniform mat4 from_merged_to_2_mat;
uniform mat4 from_merged_to_3_mat;

layout(location = 0) uniform sampler2D depth_tex_0;
layout(location = 1) uniform sampler2D depth_tex_1;
layout(location = 2) uniform sampler2D depth_tex_2;
layout(location = 3) uniform sampler2D depth_tex_3;

layout(binding = 4, r32f) uniform image2D merged_depth_tex_0;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= WIDTH || gid.y >= HEIGHT)   
        return;

    vec2 tcs[4];
    tcs[0]= (vec2(gid)+vec2(0.5))/vec2(WIDTH, HEIGHT);
    float depths[4];
    depths[0] = texture(depth_tex_0, tcs[0]).x;

    tcs[1] = tcs[0]+motion_vector(tcs[0], depths[0], from_merged_to_1_mat);
    tcs[2] = tcs[0]+motion_vector(tcs[0], depths[0], from_merged_to_2_mat);
    tcs[3] = tcs[0]+motion_vector(tcs[0], depths[0], from_merged_to_3_mat);


    depths[1] = texture(depth_tex_1, tcs[1]).x;
    depths[2] = texture(depth_tex_2, tcs[2]).x;
    depths[3] = texture(depth_tex_3, tcs[3]).x;
    float skip_tc = 0.01;
    for (int i = 0; i < 4; i++) {
        if (depths[i] <= 0.0001 || (any( lessThan(tcs[i], vec2(skip_tc))) || any(greaterThan(tcs[i], vec2(1-skip_tc)))))
            depths[i] = 1;
        else
            depths[i] = linear_depth(depths[i], NEAR, FAR);
    }

    float len_tc =0;
    float cos_01 =0;
    float weight =0;

    float merged_depth = 0;
    float sum = 0;
    int num_v = 0;
    float tdis;
    for (int i = 0; i < 4; i++) {
        tdis = MERGE_DEPTH_TDIS_SCALE*(depth_noise_var(depths[0])+depth_noise_var(depths[i]));
        len_tc = 0.9*length(2.0*tcs[i]-1);
        cos_01 = pow(0.5*cos(min(len_tc*3.1415926,3.141592))+0.5,2);
        weight = 0.25* max(0.0, max(0.0, cos_01) *depth_weight(depths[i])); // vignette_weight(1.2*tcs[i]-0.1)
        if(i == 0) weight = max(0.05, weight); 
        if (depths[i] == 1){ 
            continue;
        }
        if (depths[0] == 1){
            merged_depth += weight*depths[i];
            sum += weight;
            continue;
        }
        // // this should not happen
        // if (depths[i] < depths[0] - tdis ){
        //     continue;
        // }
        // // this is occluded 
        // if (depths[i] > depths[0] + tdis ){
        //     continue;
        // }

        if (abs(depths[i] - depths[0]) < tdis ){
            merged_depth += weight*depths[i];
            sum += weight;
            num_v++;
        }
    }
    if ( sum < 0.00001 ){
        imageStore(merged_depth_tex_0, gid, vec4(1)); //vec4(max_index-2*gid,0,1));
    } else {
        merged_depth /= sum;
        merged_depth = nonlinear_depth(merged_depth, NEAR, FAR);

        // merged_depth = nonlinear_depth(depths[0], NEAR, FAR);
        // float lin = linear_depth(merged_depth, NEAR, FAR)*20;
        imageStore(merged_depth_tex_0, gid, vec4(merged_depth)); //vec4(max_index-2*gid,0,1));
        // imageStore(merged_depth_tex_0, gid, vec4(tdis));
        // imageStore(merged_depth_tex_0, gid, vec4(num_v/4));
        // imageStore(merged_depth_tex_0, gid, vec4(100*abs(depths[1]-depths[0])));
        // imageStore(merged_depth_tex_0, gid, vec4(1000*depths[0]));
        // imageStore(merged_depth_tex_0, gid, vec4(sum));  
    }
    // imageStore(merged_depth_tex_0, gid, vec4(depths[0]));
    // merged_depth = depths[1];
    // float lin = linear_depth(merged_depth, NEAR, FAR)*30;
    // imageStore(merged_depth_tex_0, gid, vec4(lin)); //vec4(max_index-2*gid,0,1));
    // vec2 motion_vec = motion_vector(tc_cur, depth_cur, from_cur_to_last_mat);
    // // float depth_last = texture(depth_img_last,tc_last).x;
    // // float depth_diff = depth_cur-depth_last;

    // imageStore(motion_vectors_from_cur_to_last, gid, vec4(motion_vec,0,1)); //vec4(max_index-2*gid,0,1));

    return;
}
