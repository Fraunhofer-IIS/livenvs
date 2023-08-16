#version 130
in vec2 in_tc;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform sampler2D depth_tex;
uniform vec3 tgt_cam_pos_ws;
uniform vec3 src_cam_pos_ws;

out vec2 tc;
out vec3 tgt_view_dir_ws;
out vec3 src_view_dir_ws;
void main() {
    float depth = 2*texture(depth_tex,in_tc).r-1;
    depth = clamp(depth, -0.99, 0.99);
    vec4 pos_wc = model * vec4(2*in_tc-1,depth, 1.0);
    pos_wc /= pos_wc.w;
    src_view_dir_ws = normalize(src_cam_pos_ws-pos_wc.xyz);
    tgt_view_dir_ws = normalize(tgt_cam_pos_ws-pos_wc.xyz);
    gl_Position = view * pos_wc;
    gl_Position = proj * gl_Position;
    tc = in_tc;
}
