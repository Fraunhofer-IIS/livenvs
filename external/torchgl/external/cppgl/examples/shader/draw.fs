#version 330
in vec4 pos_wc;
in vec3 norm_wc;
in vec2 tc;

layout (location = 0) out vec4 out_col;

uniform sampler2D diffuse;

void main() {
    out_col = vec4(texture(diffuse, tc).rgb, 1); // lookup diffuse texture
    out_col = vec4(norm_wc, 1); // world-space normal in [0, 1]
}
