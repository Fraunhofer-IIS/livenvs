#version 130
in vec2 in_tc;

uniform mat4 model; // this is (P*V)^1 of the src view
uniform mat4 view;
uniform mat4 proj;
uniform sampler2D depth_tex;
uniform vec3 tgt_cam_pos_ws;
uniform vec3 src_cam_pos_ws;

out vec3 pos_wc;
out float depth;
out vec3 gs_debug;
out vec2 gs_tc;
out vec3 gs_tgt_view_dir_ws;
out vec3 gs_src_view_dir_ws;
void main() {
    depth = 2*texture(depth_tex,in_tc).r-1;
    vec4 pos = model * vec4(2*in_tc-1,depth, 1.0);
    pos /= pos.w;
    pos_wc = pos.xyz;
    // vec4 z = vec4(0,0,-texture(depth_tex,in_tc).r,1);
    // z = proj*z;
    // z /= z.w;
    // vec4 pos = model * vec4(2*in_tc-1,z.z, 1.0);
    // pos /= pos.w;
    // pos_wc = pos.xyz;

    gs_src_view_dir_ws = normalize(src_cam_pos_ws-pos_wc);
    gs_tgt_view_dir_ws = normalize(tgt_cam_pos_ws-pos_wc);
    gl_Position = view * pos;
    gl_Position = proj * gl_Position;
    gs_tc = in_tc;
}
