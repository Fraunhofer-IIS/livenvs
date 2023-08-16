#version 130
in vec3 in_pos;
in vec2 in_tc;
uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
out vec2 tc;
void main() {
    gl_Position = proj * view * model * vec4(in_pos, 1.);
    tc = in_tc;
}
