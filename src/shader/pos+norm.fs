#version 130
in vec4 pos_wc;
in vec3 norm_wc;
out vec4 out_col;
out vec3 out_pos;
out vec3 out_norm;
uniform vec3 color;
void main() {
    out_col = vec4(color, 1);
    out_pos = pos_wc.xyz;
    out_norm = norm_wc;
}
