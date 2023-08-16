#version 330
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_tc;

out vec2 tc;

void main() {
    vec4 pos= vec4(vec3(2.0)*in_pos - vec3(1.0), 1.0);
    pos.z = -0.8;
    gl_Position = pos;
    tc = in_tc;
}
