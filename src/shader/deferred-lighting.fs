#version 130
in vec2 tc;
out vec4 out_col;
uniform sampler2D gbuf_depth;
uniform sampler2D gbuf_diff;
uniform sampler2D gbuf_spec;
uniform sampler2D gbuf_pos;
uniform sampler2D gbuf_norm;
uniform vec3 ambient_col;
uniform vec3 light_dir;
uniform vec3 light_col;
uniform vec3 cam_pos;
uniform vec2 near_far;
uniform vec2 screenres;

float linear_depth(in float depth, in float near, in float far) { return (2.0 * near) / (far + near - depth * (far - near)); }

float sobel_edges() {
    float dx = 2.f / screenres.x;
    float dy = 2.f / screenres.y;
    float mag_norm = 0, mag_depth = 0, mag_col = 0;
    {
        vec3 norm = texture(gbuf_norm, tc).rgb;
        // horizontal
        float Gx = 0;
        Gx += -1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(-dx, dy)).rgb));
        Gx += -2 * abs(dot(norm, texture(gbuf_norm, tc + vec2(-dx, 0)).rgb));
        Gx += -1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(-dx, -dy)).rgb));
        Gx += 1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(dx, dy)).rgb));
        Gx += 2 * abs(dot(norm, texture(gbuf_norm, tc + vec2(dx, 0)).rgb));
        Gx += 1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(dx, -dy)).rgb));
        Gx /= 8.f;
        // vertical
        float Gy = 0;
        Gy += -1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(-dx, dy)).rgb));
        Gy += -2 * abs(dot(norm, texture(gbuf_norm, tc + vec2(0, dy)).rgb));
        Gy += -1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(dx, dy)).rgb));
        Gy += 1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(-dx, -dy)).rgb));
        Gy += 2 * abs(dot(norm, texture(gbuf_norm, tc + vec2(0, -dy)).rgb));
        Gy += 1 * abs(dot(norm, texture(gbuf_norm, tc + vec2(dx, -dy)).rgb));
        Gy /= 8.f;
        mag_norm = sqrt(Gx*Gx + Gy*Gy);
    }
    {
        // horizontal
        float Gx = 0;
        Gx += -1 * linear_depth(texture(gbuf_depth, tc + vec2(-dx, dy)).r, near_far.x, near_far.y);
        Gx += -2 * linear_depth(texture(gbuf_depth, tc + vec2(-dx, 0)).r, near_far.x, near_far.y);
        Gx += -1 * linear_depth(texture(gbuf_depth, tc + vec2(-dx, -dy)).r, near_far.x, near_far.y);
        Gx += 1 * linear_depth(texture(gbuf_depth, tc + vec2(dx, dy)).r, near_far.x, near_far.y);
        Gx += 2 * linear_depth(texture(gbuf_depth, tc + vec2(dx, 0)).r, near_far.x, near_far.y);
        Gx += 1 * linear_depth(texture(gbuf_depth, tc + vec2(dx, -dy)).r, near_far.x, near_far.y);
        Gx /= 8.f;
        // vertical
        float Gy = 0;
        Gy += -1 * linear_depth(texture(gbuf_depth, tc + vec2(-dx, dy)).r, near_far.x, near_far.y);
        Gy += -2 * linear_depth(texture(gbuf_depth, tc + vec2(0, dy)).r, near_far.x, near_far.y);
        Gy += -1 * linear_depth(texture(gbuf_depth, tc + vec2(dx, dy)).r, near_far.x, near_far.y);
        Gy += 1 * linear_depth(texture(gbuf_depth, tc + vec2(-dx, -dy)).r, near_far.x, near_far.y);
        Gy += 2 * linear_depth(texture(gbuf_depth, tc + vec2(0, -dy)).r, near_far.x, near_far.y);
        Gy += 1 * linear_depth(texture(gbuf_depth, tc + vec2(dx, -dy)).r, near_far.x, near_far.y);
        Gy /= 8.f;
        mag_depth = sqrt(Gx*Gx + Gy*Gy);
    }
    float magnitude = max(pow(20 * mag_norm, 10), pow(50 * mag_depth, 10));
    return magnitude > 1 ? 0 : 1;
}

void main() {
    float depth = texture(gbuf_depth, tc).r;
    gl_FragDepth = depth;
    vec3 norm = texture(gbuf_norm, tc).xyz;
    if (depth == 1.f || norm == vec3(0,0,0))  { discard; return; }

    // ambient and diffuse
    float n_dot_l = max(0, dot(norm, -light_dir));
    vec3 diff = texture(gbuf_diff, tc).rgb;
    out_col = vec4(diff * (ambient_col + light_col * n_dot_l), 1);
    // specular
    vec3 pos_wc = texture(gbuf_pos, tc).xyz;
    vec3 v = normalize(cam_pos - pos_wc.xyz);
    float n_dot_h = max(0, dot(norm, normalize(-light_dir + v)));
    out_col.xyz += pow(n_dot_h, 25)* texture(gbuf_spec, tc).rgb; 


    // // comic style shading
    // n_dot_l = int(4 * n_dot_l) / 4.f;
    // // comic style corners
    // float edge_f = sobel_edges();
    // // lighting
    // vec3 diff = texture(gbuf_diff, tc).rgb;
    // out_col = vec4(edge_f * diff * (ambient_col + light_col * n_dot_l), 1);
}
