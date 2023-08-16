
#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable

uniform int view_index;
uniform int num_patches_w; // cur_w
uniform int num_patches_h; // cur_h
uniform bool culled;

layout(binding = 0, r16f) uniform image2D input_level;
layout(binding = 1, r32f) uniform image2D vs_score_tex;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 in_lvl_size = imageSize(input_level);
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= num_patches_w || gid.y >= num_patches_h)
        return;
    int lin_index = gid.x*num_patches_h + gid.y;

    if (culled){
        imageStore(vs_score_tex, ivec2(lin_index, view_index), vec4(0.0));  // TODO 
        // TODO set to 0
        return;
    } 

    float score = imageLoad(input_level, gid).x;

    imageStore(vs_score_tex, ivec2(lin_index, view_index), vec4(score));  // TODO optimize redundant load call
    if(gid.x >= in_lvl_size.x || gid.y >= in_lvl_size.y)
        imageStore(vs_score_tex, ivec2(lin_index, view_index), vec4(0.2));  // TODO optimize redundant load call

	return;
}
