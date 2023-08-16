#pragma once

#include "torchgl.h"

# include "camera.h"
# include "import_utils/cv_utils.h"
# include "import_utils/import_utils.h"

#include "single-view-geometry.h"


struct ViewSelector {
    ViewSelector();

    enum SelectorMode {
        ALL,
        SINGLE_VIEW,
        CLOSEST,
        FOLLOW
    };

    SelectorMode mode = CLOSEST;

    // single view mode stuff
    std::map<std::string, SingleViewGeometry>::const_iterator current_single_view;
    const std::string& get_current_single_view_name() const;
    void prev_single_view();
    void next_single_view();
    void set_mode(const SelectorMode& mode);
    void set_current_follow_cam(const CV_GL_Camera& cv_cam);
    void reset();

    // closest view mode stuff
    float dist_factor = 1.f;
    float dot_factor = 1.f;
    int mipmap_for_patchification = 3;
    int current_num_patches = 0;
    int current_num_patches_w = 0;
    int current_num_patches_h = 0;

    //if FOLLOW select camera id in gui 
    // else current_camera() is used 
    void sort_views();
    // void select_closest(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos);
    void sort_according_to_cam_pose(const glm::vec3& pos, const glm::vec3& dir);
    void sort_according_to_coverage(const Camera& cppgl_cam);

    std::vector<std::pair<std::string, std::string>> get_current_closest_names() const;
    std::vector<SingleViewGeometry> get_closest() const;
        

    void update_view_geoms_from_svg_map();
    
    // data
    std::vector<SingleViewGeometry> view_geoms;
    std::vector<SingleViewGeometry> static_view_geoms;
    const std::vector<SingleViewGeometry>& get_all_static() const;


    // view selection
    bool mipmap = false;
    bool cull_frustum = true;
    int num_closest_views = 15;

    int cur_follow_cam = 0;
    int min_cam_id = 0;
    int max_cam_id = 0;

    bool show_frusta = false;
    static void debug_gui();
    static void control_gui();
};