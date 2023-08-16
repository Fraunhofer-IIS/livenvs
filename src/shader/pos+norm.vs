#version 130
in vec3 in_pos;
in vec3 in_norm;
uniform mat4 model;
uniform mat4 model_normal;
uniform mat4 view;
uniform mat4 proj;
out vec4 pos_wc;
out vec3 norm_wc;
void main() {
    pos_wc = model * vec4(in_pos, 1.0);
    norm_wc = in_norm; //normalize(mat3(model_normal) * in_norm);
    gl_Position = proj * view * pos_wc;
}
