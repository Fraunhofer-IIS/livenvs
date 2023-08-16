#version 130
#include "shared-helpers.glsl"

in vec2 tc;
out vec4 out_col;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform vec2 near_far;

float linear_depth(in float depth, in float near, in float far) { return (2.0 * near) / (far + near - depth * (far - near)); }

void main() {
    if (tc.x < 0.5) {
        if (tc.y < 0.5)
            out_col = vec4(texture(tex0, tc * 2));
        else
            out_col = vec4(texture(tex1, vec2(tc.x * 2, (tc.y - 0.5) * 2)).rgb, 1);
    } else {
        if (tc.y < 0.5)
            out_col = vec4(texture(tex2, vec2((tc.x - 0.5) * 2, tc.y * 2)).rgb, 1);
        else
            out_col = vec4(texture(tex3, (tc - 0.5) * 2).rgb, 1);
    }
}
