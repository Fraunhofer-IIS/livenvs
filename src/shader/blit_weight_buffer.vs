#version 130
in vec3 in_pos;
in vec2 in_tc;

uniform mat4 inv_proj_view;
uniform vec3 tgt_cam_pos_ws;

out vec2 tc;
void main() {
    vec4 pos= vec4(vec3(2.0)*in_pos - vec3(1.0), 1.0);
    pos.z = -0.8;
    gl_Position = pos;
    tc = in_tc;
}
