
#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "shared-helpers.glsl"
#include "buffers.glsl"

uniform int WIDTH;
uniform int HEIGHT;

uniform sampler2DArray pyramid_fused_feat;

layout(binding = 0, rgba16f) uniform image2D output_image0;
layout(binding = 1, rgba16f) uniform image2D output_image1;
layout(binding = 2, rgba16f) uniform image2D output_image2;
layout(binding = 3, rgba16f) uniform image2D output_image3;
layout(binding = 4, r32f) uniform image2D output_depth;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if(gid.x >= WIDTH || gid.y >= HEIGHT)
        return;

    float d = 1000.0;
    int level = 0;
    ivec2 sample_index = gid*2;
    ivec2 cur_dims = ivec2(WIDTH, HEIGHT);
    for(; int(WIDTH / pow(2, level)) > 0; level++) {
        if (level > MAX_HOLE_FILLING_LEVEL) {
            imageStore(output_image0, gid, vec4(-0.2)); //vec4(max_index-2*gid,0,1));
            imageStore(output_image1, gid, vec4(-0.02)); //vec4(max_index-2*gid,0,1));
            imageStore(output_image2, gid, vec4(-0.02)); //vec4(max_index-2*gid,0,1));
            imageStore(output_image3, gid, vec4(-0.02)); //vec4(max_index-2*gid,0,1));
            imageStore(output_depth, gid, vec4(1));
            return; 
        }
        sample_index = clamp(sample_index/2, ivec2(0), cur_dims-ivec2(1)); 
        d = texelFetch(pyramid_fused_feat, ivec3(sample_index,4), level).r; //ivec2(gid/pow(2,level))
        if (d < 1.0) break; 
        cur_dims /= 2;
    }
    vec4 feat0 = texelFetch(pyramid_fused_feat, ivec3(sample_index,0), level);
    imageStore(output_image0, gid, feat0); //vec4(max_index-2*gid,0,1));
    vec4 feat1 = texelFetch(pyramid_fused_feat, ivec3(sample_index,1), level);
    imageStore(output_image1, gid, feat1); //vec4(max_index-2*gid,0,1));
    vec4 feat2 = texelFetch(pyramid_fused_feat, ivec3(sample_index,2), level);
    imageStore(output_image2, gid, feat2); //vec4(max_index-2*gid,0,1));
    vec4 feat3 = texelFetch(pyramid_fused_feat, ivec3(sample_index,3), level);
    imageStore(output_image3, gid, feat3); //vec4(max_index-2*gid,0,1));
    imageStore(output_depth, gid, vec4(d)); //vec4(max_index-2*gid,0,1));

	return;
}
