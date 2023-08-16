#version 130
in vec2 tc;
uniform sampler2D tex;
uniform sampler2D alpha_tex;
uniform vec4 color;
out vec4 out_col;
void main() {
    float alpha = texture(alpha_tex, tc).r;
    out_col = alpha*texture(tex, tc)+(1-alpha)*color;
}
