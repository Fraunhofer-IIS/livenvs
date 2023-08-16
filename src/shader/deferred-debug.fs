#version 130
in vec2 tc;
out vec4 out_col;
uniform sampler2D gbuf_depth;
uniform sampler2D gbuf_diff;
uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norm;
uniform vec3 ambient_col;
uniform vec3 light_dir;
uniform vec3 light_col;
uniform vec2 near_far;

float linear_depth(in float depth, in float near, in float far) { return (2.0 * near) / (far + near - depth * (far - near)); }

void main() {
    if (tc.x < 0.5) {
        if (tc.y < 0.5)
            out_col = vec4(linear_depth(texture(gbuf_depth, tc * 2).r, near_far.x, near_far.y));
        else
            out_col = vec4(texture(gbuf_diff, vec2(tc.x * 2, (tc.y - 0.5) * 2)).rgb, 1);
    } else {
        if (tc.y < 0.5)
            out_col = vec4(texture(gbuf_pos, vec2((tc.x - 0.5) * 2, tc.y * 2)).rgb, 1);
        else
            out_col = vec4(texture(gbuf_norm, (tc - 0.5) * 2).rgb, 1);
    }
}
