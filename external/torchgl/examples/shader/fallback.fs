#version 130
in vec2 tc;
out vec4 out_col;
void main() {
    out_col = vec4(tc,0,1);
}
