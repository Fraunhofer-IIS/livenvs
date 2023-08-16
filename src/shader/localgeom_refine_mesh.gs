#version 430
#include "shared-helpers.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 pos_wc[3];
in float depth[3];
in vec3 gs_debug[3];
in vec2 gs_tc[3];
in vec3 gs_src_view_dir_ws[3];
in vec3 gs_tgt_view_dir_ws[3];
uniform vec3 src_cam_pos;
uniform float cut_off_depth;
out vec2 tc;
out vec3 view_dir;
out vec3 debug;
out vec3 src_view_dir_ws;
out vec3 tgt_view_dir_ws;
out vec3 fs_pos_wc;

void discard_triangle(bool debug){
    if(!debug) return;
    for (uint i = 0; i < 3; ++i){
        gl_Position = gl_in[i].gl_Position;
        tc = vec2(0);
        view_dir = vec3(0);
        EmitVertex();
    }
    EndPrimitive();
}


void main() {

    bool debug_mode = false; //true;

    // discard triangles that have a vertex of depth == 1
    for (uint i = 0; i < 3; ++i)
        if (1000*linear_depth(depth[i], 0.01,1000) > cut_off_depth) {
            discard_triangle(debug_mode);
            return;
        }

    // discard illformed triangles at ocllusion edges
    // metric:  
    vec3 dir_01 = normalize(pos_wc[1]-pos_wc[0]);
    vec3 dir_02 = normalize(pos_wc[2]-pos_wc[0]);
    vec3 dir_12 = normalize(pos_wc[2]-pos_wc[1]);
    // float cos_alpha = abs(dot(dir_01, dir_02));
    // float cos_beta = abs(dot(-dir_12, -dir_02));
    // float cos_gamma = abs(dot(-dir_01, dir_12));
    // float angle_threshold = .992;
    // if (cos_alpha > angle_threshold ||
    //     cos_beta > angle_threshold ||
    //     cos_gamma > angle_threshold ){
    //         discard_triangle(debug_mode);
    //         return;
    // }

    // calc triangle norms to kick out tris that do not face the camera
    vec3 norm_wc = normalize(cross(dir_02, dir_01));
    float n_dot_v = dot(-norm_wc, gs_src_view_dir_ws[0]);
    if (n_dot_v < 0.12){
            discard_triangle(debug_mode);
            return;
    }

    // emit triangles that pass quality constraints
    for (uint i = 0; i < 3; ++i){
        gl_Position = gl_in[i].gl_Position;
        tc = gs_tc[i];
        src_view_dir_ws = gs_src_view_dir_ws[i];
        tgt_view_dir_ws = gs_tgt_view_dir_ws[i];
        fs_pos_wc = pos_wc[i];
        debug = vec3(n_dot_v, linear_depth(depth[i], 0.01,1000), 0); //norm; //gs_debug[i];
        EmitVertex();
    }
    
    EndPrimitive();
}
