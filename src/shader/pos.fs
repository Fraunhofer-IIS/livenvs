#version 130
in vec4 pos_wc;
in vec2 tc;
out vec4 out_col;
uniform vec3 color;
uniform sampler2D tex;
void main() {
    out_col = texture(tex,tc); //vec4(tc, 0,1); //pos_wc;//vec4(1,0,0,1);
}
