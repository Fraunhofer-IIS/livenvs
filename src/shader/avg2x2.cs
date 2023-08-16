
#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"

uniform int WIDTH;
uniform int HEIGHT;


layout(binding = 0, r16f) uniform image2D input_level;
layout(binding = 1, r16f) uniform image2D output_level;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 in_lvl_size = imageSize(input_level);
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= WIDTH || gid.y >= HEIGHT)
        return;
    ivec2 index = 2*gid;
    float sum = 0;
    for(int x = 0; x < 2; ++x){
        for(int y = 0; y < 2; ++y){
            ivec2 i = clamp(index+ivec2(x,y),ivec2(0), in_lvl_size-ivec2(1)); 
            sum += imageLoad(input_level, i).r*0.25;
        }
    }
    imageStore(output_level, gid, vec4(sum));  // TODO optimize redundant load call

	return;
}
