#pragma once
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

struct globals_n_gui {
    bool show_project_gui = false;
    const float NEAR = 0.01, FAR = 1000.f;

    // NOTE: currently, only (1296,968), (1280, 720), (1920, 1080) are supported as (WIDTH, HEIGHT)
    // scannet
    // const uint32_t WIDTH = 1296; 
    // const uint32_t HEIGHT = 968;
    // glm::vec3 cut_off_depth = glm::vec3(4.0, 0.001, 50.0);             
    // glm::vec3 cut_off_depth_deferred = glm::vec3(4.5, 0.001, 50.0);  
    // glm::vec3 beta_tgt = glm::vec3(0.00175, 0.f, 0.005f); 
    // glm::vec3 beta_src = glm::vec3(0.0025, 0.f, 0.005f);

    //zed outdoor / large scale
    const uint32_t WIDTH = 1280; 
    const uint32_t HEIGHT = 720;
    glm::vec3 cut_off_depth = glm::vec3(12.0, 0.001, 50.0);             
    glm::vec3 cut_off_depth_deferred = glm::vec3(10.0, 0.001, 50.0);     
    glm::vec3 beta_tgt = glm::vec3(0.000243, 0.f, 0.005f); 
    glm::vec3 beta_src = glm::vec3(0.000208, 0.f, 0.005f); 

    // recording stuff
    bool toggle_datasets = false;
    bool record_demo = true;
    bool use_demo_cam = false;
    bool play_demo = false;
    bool play_animation = false;
    bool merge_depth = false;
    bool render_traj = false;
    bool render_anim = false;
    bool render_depths_of_traj = false;
    bool write_depths_of_traj = false;
    bool use_filtered_depthmaps = false;
    uint32_t merge_depth_bundle_index = 0;


    // warp stuff
    const uint32_t LOCAL_GEOM_HQ_WIDTH = 1*WIDTH;
    const uint32_t LOCAL_GEOM_HQ_HEIGHT = 1*HEIGHT;
    const uint32_t LOCAL_GEOM_LQ_WIDTH = 256;
    const uint32_t LOCAL_GEOM_LQ_HEIGHT = 256*(float(HEIGHT)/float(WIDTH));
    bool culling= true; // enabled by default in context
    bool hole_filling= false;
    glm::vec2 betas= glm::vec2(1);   
    int weight_exponent = 5;
    bool use_vignette_weight = true;
    bool use_depth_weight = true;
    bool use_view_weight = true;
    float inf_depth_deferred = 25.0;
    bool use_background_deferred = true;
    bool mask_background = false;

    //nns
    bool sync_active= true;
    float temporal_feedback_weight = 0.1;

    // render mode
    bool render_aux= false;
    bool deferred = false;

    void gui_callback();
};
