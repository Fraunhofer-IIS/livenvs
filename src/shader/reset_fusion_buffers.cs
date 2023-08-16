
#version 450
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"


uniform int WIDTH;
uniform int HEIGHT;

layout(local_size_x = 16, local_size_y = 16) in;
 void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    if(storePos.x >= WIDTH || storePos.y >= HEIGHT)
        return;
    int index = int(storePos.x) + int(storePos.y) * WIDTH;

    counter[index] = 0;
    locks[index] = 0;

    fragmentdata[index].tdis = .0f; // needs to be 0!
    fragmentdata[index].src_view_dir = vec3(1);
    fragmentdata[index].depth = 1000.0-1e-10; //== FAR!! will be overwritten by first valid fragment if incoming depth is smaller (usually always)
    fragmentdata[index].weight = -42.0; // will always be overwritten by first valid fragment
    fragmentdata[index].padding = 42.0f;
    fragmentdata[index].def_weight_sum = -100.0f;
    fragmentdata[index].feat = debug_feat(vec4(0));
    fragmentdata[index].var_feat = debug_feat(vec4(0));
}
