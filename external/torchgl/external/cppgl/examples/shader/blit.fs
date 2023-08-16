#version 330
in vec2 tc;
layout (location = 0) out vec4 out_col;
uniform sampler2D tex;

void main() {
    out_col = vec4(texture(tex, tc).rgb,1);
}
