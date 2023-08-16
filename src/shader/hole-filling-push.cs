
#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"

uniform int WIDTH;
uniform int HEIGHT;

layout(binding = 6, rgba16f) uniform image2DArray input_feat;
layout(binding = 7, rgba16f) uniform image2DArray output_feat;

ivec2 argmin_depth(ivec2 index){
    float cur_min = 100.0;
    ivec2 cur_index = index;
    for(int x = 0; x < 2; ++x){
        for(int y = 0; y < 2; ++y){
            ivec2 i = index+ivec2(x,y); 
            float d = imageLoad(input_feat, ivec3(i,4)).r; // depth is in the 4th layer
            if(d < cur_min){
                cur_min = d;
                cur_index = i;
            }
        }
    }
    
    return cur_index;
}

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= WIDTH || gid.y >= HEIGHT)
        return;
        
    // int index = int(gid.x) + int(gid.y) * WIDTH;
    ivec2 max_index = argmin_depth(2*gid);
    const int layers = 4;
    for(int l = 0; l < 5; ++l){
        vec4 feat0 = imageLoad(input_feat, ivec3(max_index,l));
        imageStore(output_feat, ivec3(gid,l), feat0); //vec4(max_index-2*gid,0,1));
    }
    // imageStore(output_feat, ivec3(gid,4), imageLoad(input_feat, ivec3(max_index,4)));  // TODO optimize redundant load call


	return;
}
