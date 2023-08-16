#define NEAR 0.01
#define FAR 1000.0

// 0.01 is minimum weight from svs
#define MIN_FUSE_WEIGHT 1e-16
#define DEPTH_WEIGHT_SCALE 10.0


#define FUSE_FRAG_TDIS_BETA 0.9
#define MERGE_DEPTH_TDIS_SCALE  0.001


#define MAX_HOLE_FILLING_LEVEL 2

float depth_noise_var(float linear_depth){
    float depth_in_meters = linear_depth * FAR; // multply by far
     // couple variance to depth according to https://www.researchgate.net/publication/261601243_Characterizations_of_Noise_in_Kinect_Depth_Images_A_Review Table 2
    return    max(0,(9.0 * depth_in_meters*depth_in_meters + 0.74*depth_in_meters-0.58)*0.001); 
}

float calc_tdis(float linear_depth_tgt, float tgt_scale, float linear_depth_src, float src_scale){
    return tgt_scale*depth_noise_var(linear_depth_tgt) + src_scale*depth_noise_var(linear_depth_src);
}

float linear_depth(in float depth, in float near, in float far) { return (2.0 * near) / (far + near - depth * (far - near)); }

float nonlinear_depth(in float lin_depth01, in float near, in float far){
    float lin_depth = (far-near)*lin_depth01;//+near;
    return (((-far - near) / (far - near))* (-1*lin_depth) +
                    (-2.0*far*near/(far-near))) /(1.f*lin_depth);
}

vec2 motion_vector(const in vec2 tc_cur, const in float depth_cur, const in mat4 from_cur_to_last_mat ){
    vec3 ndc3 = 2*vec3(tc_cur, depth_cur)-1;
    vec4 ndc = vec4(ndc3,1);
    ndc = from_cur_to_last_mat*ndc;
    ndc/=ndc.w;
    vec2 tc_last = 0.5*ndc.xy+0.5;
    return tc_last-tc_cur;
}

float view_dir_weight(vec3 tgt_view_dir_ws, vec3 src_view_dir_ws){
    return max(0,dot(tgt_view_dir_ws, src_view_dir_ws));
}

float depth_weight(float lin_src_depth){
    return 1.0 / (DEPTH_WEIGHT_SCALE*depth_noise_var(lin_src_depth));
}

float vignette_weight(vec2 tc){
    vec2 tc_11 = 2*tc -1;
    float vignette_weight =  pow(max(0,1-1.2*length(tc_11)/length(vec2(1))),2.0);
    return vignette_weight;
}
