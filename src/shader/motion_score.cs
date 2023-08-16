
#version 450
#include "shared-helpers.glsl"

uniform int WIDTH;
uniform int HEIGHT;

uniform sampler2D depth_img_last;
uniform sampler2D depth_img_cur;

uniform mat4 from_cur_to_last_mat;

layout(binding = 0, r16f) uniform image2D motion_vectors_from_cur_to_last;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= WIDTH || gid.y >= HEIGHT)
        return;

    vec2 tc_cur = (vec2(gid)+vec2(0.5))/vec2(WIDTH, HEIGHT);
    float depth_cur = texture(depth_img_cur, tc_cur).x;
    if (depth_cur <= 0.0001){
        imageStore(motion_vectors_from_cur_to_last, gid, vec4(0.1,0,0,1)); //vec4(max_index-2*gid,0,1));
        return;
    }
    
    vec2 motion_vec = motion_vector(tc_cur, depth_cur, from_cur_to_last_mat);
    // float depth_last = texture(depth_img_last,tc_last).x;
    // float depth_diff = depth_cur-depth_last;

    imageStore(motion_vectors_from_cur_to_last, gid, vec4(vec3(20*length(motion_vec)),1)); //vec4(max_index-2*gid,0,1));

    return;
}
