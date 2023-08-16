#version 130
in vec4 pos_wc;
in vec3 norm_wc;
in vec2 tc;
out vec4 out_col;
uniform vec3 color;
uniform sampler2D tex;
void main() {
    out_col = vec4(0.2, 0.17, 0.15,1) + vec4(1.0)*max(0,dot(norm_wc, normalize(vec3(1,-1,0))));
}