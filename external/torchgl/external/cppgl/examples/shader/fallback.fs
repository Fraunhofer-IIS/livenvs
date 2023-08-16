#version 330
in vec2 tc;
layout (location = 0) out vec4 out_col;

void main() {
    out_col = vec4(tc,0,1);
}
