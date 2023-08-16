#version 430
in vec2 fs_tc;
uniform sampler2D tex;
out vec4 out_col;
void main() {
    out_col = vec4(((1+gl_Layer)/4.f)* fs_tc,0,1);
    // out_col = vec4(fs_tc,0,1);
    // out_col = vec4(0,1,0,1);
}
