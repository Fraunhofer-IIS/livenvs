#version 130
in vec3 in_pos;
in vec3 in_norm;
in vec2 in_tc;
uniform mat4 model;
uniform mat4 model_normal;
uniform mat4 model_view;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 cam_pos_ws;
out vec4 pos_wc;
out vec3 view_wc;
out vec3 view_vc;
out vec3 norm_wc;
out vec3 norm_v;
out vec2 tc;
void main() {
    pos_wc = model * vec4(in_pos, 1.0);
    pos_wc /= pos_wc.w;
    view_wc = normalize(pos_wc.xyz-cam_pos_ws);
    norm_wc = normalize(mat3(model_normal) * in_norm);
    norm_v = normalize(mat3(model_view)*in_norm);
    tc = in_tc;
    gl_Position = view * pos_wc;
    view_vc = -normalize(gl_Position.xyz);
    gl_Position = proj * gl_Position;
}
