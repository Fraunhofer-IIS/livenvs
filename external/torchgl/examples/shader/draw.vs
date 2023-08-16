#version 130
in vec3 in_pos;
in vec3 in_norm;
in vec2 in_tc;
uniform mat4 model;
uniform mat4 model_normal;
uniform mat4 view;
uniform mat4 proj;
out vec4 pos_wc;
out vec3 norm_wc;
out vec2 tc;
void main() {
    pos_wc = model * vec4(in_pos, 1.0);
    norm_wc = normalize(mat3(model_normal) * in_norm);
    tc = in_tc;
    gl_Position = proj * view * pos_wc;
}
