#version 430
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable
#include "buffers.glsl"
#include "shared-helpers.glsl"
in vec2 tc;

uniform int WIDTH;
uniform int HEIGHT;
uniform int exponent;

out vec4 weight_col;

vec4 colormap(float value){
    
    if(value < 0){
        return vec4(0.5,0.5,0.5,1 );
    }
    float scale = pow(10,exponent+1);
    value = scale*((1/scale)-value);

    float length_hot = 10;
    vec3 map_hot[10] = {vec3(0.3,1.0,0.2), vec3(0.8,1.0,0.8),vec3(0.8,1.0,0.8), vec3(0.8,1.0,0.8), vec3(0.9,1.0,0.9),vec3(1), vec3(1), vec3(1), vec3(1,0.8,0.8), vec3(1,0,0)};

    float findex = value*(length_hot-1);
    int low_col = int(clamp(floor(findex), 0, length_hot-1));  
    int high_col = int(clamp(ceil(findex), 0, length_hot-1));  
    
    vec3 color = mix(map_hot[low_col], map_hot[high_col], fract(findex));

    return vec4(color,1);
}


void main() {

    ivec2 fragCoord = ivec2(round(max(vec2(0),tc * vec2(WIDTH, HEIGHT)-0.5)));
    uint index = int(fragCoord.x) + int(fragCoord.y) * WIDTH;
    
    weight_col = vec4(colormap(fragmentdata[index].weight));
}
