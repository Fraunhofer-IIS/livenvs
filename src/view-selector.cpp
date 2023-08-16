#include "view-selector.h"
#include "globals+gui.h"
#include <execution>

extern globals_n_gui gg;

std::shared_ptr<ViewSelector> view_selector;

ViewSelector::ViewSelector(){
    update_view_geoms_from_svg_map();
}

static std::map<std::string, SingleViewGeometry> dummy_map;

void ViewSelector::update_view_geoms_from_svg_map(){
    if (dummy_map.empty()){
        dummy_map = std::map<std::string, SingleViewGeometry>({{"dummy", SingleViewGeometry("dummy")}});
        SingleViewGeometry::erase("dummy");
    }
    current_single_view = SingleViewGeometry::map.size() ?
        SingleViewGeometry::map.begin() : dummy_map.begin();
    view_geoms.clear();

    for (const auto& [key, val]: SingleViewGeometry::map){
        view_geoms.push_back(val);
    }
    static_view_geoms.clear();
    for (const auto& [key, val]: SingleViewGeometry::map){
        static_view_geoms.push_back(val);
    }

    min_cam_id = -1;
    max_cam_id = INT_MAX;
    for (const auto& [key, val]: SingleViewGeometry::map){
        min_cam_id = std::min(min_cam_id, int(val->cam.cam_id));
        max_cam_id = std::max(min_cam_id, int(val->cam.cam_id));
    }
}

void ViewSelector::reset(){
    if (dummy_map.empty()){
        dummy_map = std::map<std::string, SingleViewGeometry>({{"dummy", SingleViewGeometry("dummy")}});
        SingleViewGeometry::erase("dummy");
    }
    current_single_view =  dummy_map.begin();
    view_geoms.clear();
}

void ViewSelector::set_mode(const SelectorMode& mode){
    ViewSelector::mode = mode;
}

void ViewSelector::set_current_follow_cam(const CV_GL_Camera& cv_cam){
    cur_follow_cam = cv_cam.cam_id;
}


// free view synthesis do precompute which images have the biggest overlap with the target view
// we only need a very coarse approximation as we do the per-pixel decision later on 
void ViewSelector::sort_according_to_cam_pose(const glm::vec3& pos, const glm::vec3& dir){

    std::sort(view_geoms.begin(), view_geoms.end(), [pos, dir, this](SingleViewGeometry a, SingleViewGeometry b) {
            auto pos_dist_a = glm::distance2(pos, a->cam.get_gl_campos());
            auto pos_dist_b = glm::distance2(pos, b->cam.get_gl_campos());

            // map to [0,2] --> 0 == equal, 2 = inverse
            auto dot_dist_a = -glm::dot(dir, a->cam.get_gl_viewdir()) +1;
            auto dot_dist_b = -glm::dot(dir, b->cam.get_gl_viewdir()) +1;

            // square to give less about close dot prods and throw out those beyond 90 deg angle earlier
            dot_dist_a *= dot_dist_a;
            dot_dist_b *= dot_dist_b;

            // // we give cameras that are a little behind our target cam some minor bonus
            // auto a_behind_pos = glm::min(0.f,glm::dot(dir, a->cam.get_gl_campos()-pos));
            // auto b_behind_pos = glm::min(0.f,glm::dot(dir, b->cam.get_gl_campos()-pos));

            auto dist_a = dist_factor*pos_dist_a + dot_factor*dot_dist_a; // + envelope_factor*a_behind_pos;
            auto dist_b = dist_factor*pos_dist_b + dot_factor*dot_dist_b; // + envelope_factor*b_behind_pos;

            return dist_a < dist_b;
        });
}


static void own_mip_map(Texture2D tex, uint32_t w, uint32_t h){
    static Shader avg = Shader("avg", "shader/avg2x2.cs");
    for (uint32_t wl = w/2, hl = h/2, l=0; wl > 0 || hl > 0; wl/= 2, hl /=2, ++l){
            uint32_t wl_greater_0 = std::max(1u, wl);  
            uint32_t hl_greater_0 = std::max(1u, hl);  
            avg->bind();
            avg->uniform("WIDTH", wl_greater_0);
            avg->uniform("HEIGHT", hl_greater_0);

            tex->bind_image( 0,GL_READ_WRITE, tex->internal_format, l);
            tex->bind_image( 1,GL_READ_WRITE, tex->internal_format, l+1);

            avg->dispatch_compute(wl_greater_0, hl_greater_0);

            tex->unbind_image(0);
            tex->unbind_image(1);

            avg->unbind();
            // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glMemoryBarrier( GL_ALL_BARRIER_BITS);
        }
} 


inline bool clip(const glm::vec4& posc){
    return glm::all(glm::lessThanEqual(posc, glm::vec4(1.f))) && 
            glm::all(glm::greaterThanEqual(posc, glm::vec4(-1.f)));
}

inline bool inside_frustum(const SingleViewGeometry& vg, const Camera& cppgl_cam){
    static const std::vector<glm::vec4> near_plane_posc =   {   {-1.f, 1.f,-1.f, 1.f}, {-1.f,-1.f,-1.f, 1.f}, 
                                                                { 1.f,-1.f,-1.f, 1.f}, { 1.f, 1.f,-1.f, 1.f} 
                                                            };
    std::vector<glm::vec4> frustum; 
    frustum.reserve(8);
    glm::mat4 inv_v_vg = glm::inverse(vg->cam.get_gl_view());
    glm::mat4 inv_p_v = glm::inverse(vg->cam.get_gl_proj(gg.NEAR, gg.FAR));
    for (const auto& posc : near_plane_posc) {
        glm::vec4 p_view = inv_p_v*posc;
        p_view /= p_view.w;
        p_view /= p_view.z;
        glm::vec4 min_p = glm::vec4(-vg->min_d*glm::vec3(p_view),1);
        min_p = inv_v_vg*min_p;
        frustum.push_back(min_p);
        float max_d = std::min(gg.cut_off_depth[0], vg->max_d);
        glm::vec4 max_p = glm::vec4(-max_d*glm::vec3(p_view),1);
        max_p = inv_v_vg*max_p;
        frustum.push_back(max_p);
    }

    bool is_inside = false;
    glm::vec4 center = glm::vec4(0.f);
    glm::mat4 gl_vp = cppgl_cam->proj * cppgl_cam->view;
    for (const auto& fp : frustum) {
        glm::vec4 fpc = gl_vp * fp;
        fpc /= abs(fpc.w);
        if(clip(fpc)){
            is_inside = true;
            return is_inside;
        }
        center += glm::clamp(fpc, -1.f, 1.0f);
    }
    center /= float(frustum.size());
    
    // if svg frsutum inside of bb, center of bb must be in frustum after clamping.
    center.w = 0;
    // we explicitly not use clip!  
    is_inside = is_inside || glm::all(glm::lessThan(center, glm::vec4(1.f))) && 
            glm::all(glm::greaterThan(center, glm::vec4(-1.f)));

    return is_inside;
}

static std::map< std::string, float > coverage_cache;
void ViewSelector::sort_according_to_coverage(const Camera& cppgl_cam){
    // init draw stuff

    static const int w = 256;
    static const int h = 256*(float(gg.HEIGHT)/float(gg.WIDTH));
    static Framebuffer coverage_fb;
    if (!Framebuffer::valid("coverage_fb")){
        coverage_fb = Framebuffer( "coverage_fb",w,h );
        coverage_fb->attach_depthbuffer(Texture2D("coverage_depthtex", w,h, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT));
        coverage_fb->attach_colorbuffer(Texture2D("coverage_tex", w, h, GL_R16F, GL_RED, GL_FLOAT, (void*)0, true));
        coverage_fb->check();
    }
                
    static std::map< std::string, int> name_to_index;    
    if (name_to_index.size() != view_geoms.size()){
        int i = 0;
        for (const auto& vg: view_geoms)
            name_to_index[vg->name] = i++; 
    }
    static const auto shader_coverage = Shader("coverage", 
                    "shader/localgeom_refine_mesh.vs",
                    "shader/localgeom_refine_mesh.gs",
                    "shader/localgeom_coverage.fs");


    // begin per frame stuff
    auto cur = current_camera();
    make_camera_current(cppgl_cam);

    GLfloat clear_col[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_col);
    glClearColor(0,0,0,1);

    std::vector<std::map< std::string, float >> tmp_coverage_cache;
    std::map<std::string, std::vector<float>> coverages;
    int cur_w = w;
    int cur_h = h;
    
    std::vector<std::pair<int,int>> level_dims;
    do {
        level_dims.push_back({std::max(1,cur_w), std::max(1,cur_h)});
        cur_w/=2; cur_h/=2;
    } while (cur_h > 0 || cur_w > 0);
    int level = std::max(0,int(level_dims.size())-mipmap_for_patchification);
    cur_w = level_dims[level].first;
    cur_h = level_dims[level].second;
    
    // gpu batch copy stuff
    // we pool the per-view-score results in one texture to have only one device -> host copy operation
    // thus, we don't need to wait for each copy operation between views.
    // this was faster than the async copy functionality via PPBOs
    static Texture2D vs_score_tex("view_selection_score_tex", cur_w*cur_h, view_geoms.size(), GL_R32F, GL_RED, GL_FLOAT );
    if(vs_score_tex->w != cur_w*cur_h || vs_score_tex->h != view_geoms.size()){
        Texture2D::erase("view_selection_score_tex");
        vs_score_tex = Texture2D("view_selection_score_tex", cur_w*cur_h, view_geoms.size(), GL_R32F, GL_RED, GL_FLOAT );
    }
    static Shader score_copy("vs_score_copy", "shader/vs_copy.cs");
    const bool gpu_batch_copy = true;

    int counter = 0;
    for (const auto& vg: view_geoms)
    {
        if (vg->hide || vg->name == "current_frame" || (cull_frustum && !inside_frustum(vg, cppgl_cam))) {
            coverages[vg->name] = std::vector<float>(cur_w* cur_h, 0.f);
            if(gpu_batch_copy){
                // have tex of (cur_w*cur_h) x total number of views
                // copy linearized mip map layer "layer" via cs to row of tex
                score_copy->bind();
                score_copy->uniform("view_index", name_to_index[vg->name]);
                score_copy->uniform("num_patches_w", cur_w);
                score_copy->uniform("num_patches_h", cur_h);
                score_copy->uniform("culled", true);
                coverage_fb->color_textures[0]->bind_image(0, GL_READ_WRITE, GL_R16F, level);
                vs_score_tex->bind_image(1, GL_READ_WRITE, GL_R32F);
                score_copy->dispatch_compute(cur_w, cur_h);
                coverage_fb->color_textures[0]->unbind_image(0);
                vs_score_tex->unbind_image(1);
                score_copy->unbind();
            } 
            continue;
            std::cout << vg->hide << std::endl;
        }
        
        coverages[vg->name] = std::vector<float>();

        // store current clear color and set current to black
        shader_coverage->bind();
        coverage_fb->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
        vg->draw(shader_coverage, GL_TRIANGLES, gg.cut_off_depth[0], false);
        shader_coverage->unbind();
        if (mipmap){
            coverage_fb->color_textures[0]->bind(0);
            glGenerateMipmap(GL_TEXTURE_2D); 
            coverage_fb->color_textures[0]->unbind();
        }                
        else {
            own_mip_map(coverage_fb->color_textures[0], w, h);
        }
        
        //copy per-view-score to pooled texture
        if(gpu_batch_copy){
            // have tex of (cur_w*cur_h) x total number of views
            // copy linearized mip map layer "layer" via cs to row of tex
            score_copy->bind();
            score_copy->uniform("view_index", name_to_index[vg->name]);
            score_copy->uniform("num_patches_w", cur_w);
            score_copy->uniform("num_patches_h", cur_h);
            score_copy->uniform("culled", false);
            coverage_fb->color_textures[0]->bind_image(0, GL_READ_WRITE, GL_R16F, level);
            vs_score_tex->bind_image(1, GL_READ_WRITE, GL_R32F);
            score_copy->dispatch_compute(cur_w, cur_h);
            coverage_fb->color_textures[0]->unbind_image(0);
            vs_score_tex->unbind_image(1);
            score_copy->unbind();
            glMemoryBarrier( GL_ALL_BARRIER_BITS);

        } 
        else 
            coverage_fb->color_textures[0]->copy_from_gpu(coverages[vg->name], level);

        coverage_fb->unbind();
    }

    // copy tex with pooled scores to cpu
    // for debugging: fill tmp_coverage_cache according to its layout from tex 
    // maybe do faster
    std::vector<float> coverage_tex_cpu;
    vs_score_tex->copy_from_gpu(coverage_tex_cpu);

    for(const auto& vg : view_geoms){
        for(int p = 0; p < cur_w* cur_h; p++){
            // coverages[vg->name] = 
            int index = name_to_index[vg->name];
            int lin_index = cur_w*cur_h*index+p;
            float c = coverage_tex_cpu[lin_index];

            if (p >= tmp_coverage_cache.size()) tmp_coverage_cache.push_back(std::map< std::string, float >());
            tmp_coverage_cache[p][vg->name] = c;
        }
    }

    // save size of mip  map levels for display
    current_num_patches = tmp_coverage_cache.size();
    current_num_patches_w = cur_w;
    current_num_patches_h = cur_h;

    // reset clear color
    glClearColor(clear_col[0],clear_col[1],clear_col[2],1);

    // hierachical sort
    std::map< std::string, float > second_stage_tmp_coverage_cache;
    for (auto& patch_cache : tmp_coverage_cache){

        std::sort(std::execution::par_unseq, view_geoms.begin(), view_geoms.end(), [&patch_cache](SingleViewGeometry a, SingleViewGeometry b) {
                return patch_cache[a->name] > patch_cache[b->name];
            });
        for(int i = 0; i < view_geoms.size(); ++i) {
            float patched_coverage = patch_cache[view_geoms[i]->name] > 0.f ? 
                                        patch_cache[view_geoms[i]->name]+std::max(0,int(view_geoms.size())-i): 0.f;
            // float patched_coverage = patch_cache[view_geoms[i]->name] == 1.f ? 
            //                            1.f: 0.f;
            if (second_stage_tmp_coverage_cache.count(view_geoms[i]->name)){
                if (second_stage_tmp_coverage_cache[view_geoms[i]->name] < patched_coverage)
                    second_stage_tmp_coverage_cache[view_geoms[i]->name] = patched_coverage;
            } else {
                second_stage_tmp_coverage_cache[view_geoms[i]->name] = patched_coverage;   
            }
        }
    }
    std::sort(std::execution::par_unseq, view_geoms.begin(), view_geoms.end(), [&second_stage_tmp_coverage_cache](SingleViewGeometry a, SingleViewGeometry b) {
                return second_stage_tmp_coverage_cache[a->name] > second_stage_tmp_coverage_cache[b->name];
            });

    coverage_cache = second_stage_tmp_coverage_cache;

    make_camera_current(cur);
}

std::vector<std::pair<std::string, std::string>> ViewSelector::get_current_closest_names() const{
    std::vector<std::pair<std::string, std::string>> current_names;
    for(const auto& cvg : get_closest())
        current_names.push_back({cvg->name, std::to_string(coverage_cache[cvg->name])});
    return current_names;
}

void ViewSelector::sort_views(){
    static int ft = 0;
    // check whether there are new views or less
    // update accordingly
    if (view_geoms.size() != SingleViewGeometry::map.size())
        update_view_geoms_from_svg_map();

    switch (mode){
        case SINGLE_VIEW:
            break;
        case ALL:
        case CLOSEST:
            sort_according_to_coverage(current_camera());
            break;
        case FOLLOW: {
            int cam_id = cur_follow_cam;
            std::sort(std::execution::par_unseq, view_geoms.begin(), view_geoms.end(), [&cam_id](SingleViewGeometry a, SingleViewGeometry b) {
                int dista = int(cam_id) - int(a->cam.cam_id);
                int distb = int(cam_id) - int(b->cam.cam_id);
                if (dista < 0) dista += 10000;
                if (distb < 0) distb += 10000;
                return dista < distb;
            });
            break;
        }
        default:
            break;
    }
}


std::vector<SingleViewGeometry> ViewSelector::get_closest() const {

    std::vector<SingleViewGeometry> current;
    switch (mode){
        default:
        case ALL:
            current =  view_geoms; 
            break;
        case SINGLE_VIEW:
            current = {current_single_view->second};
            break;
        case CLOSEST:
        {
            auto it = view_geoms.begin();
            std::advance(it, std::min(view_geoms.size(), (size_t) num_closest_views));
            std::vector<SingleViewGeometry> closest(view_geoms.begin(), it);
            // for(int i = 0; i < std::min(view_geoms.size(), (size_t) num_closest_views); ++i) {
            //     closest.push_back(view_geoms[i]);        
            // }
            current = closest;
            break;
        }  
        case FOLLOW:
        {
            std::vector<SingleViewGeometry> closest;
            int cam_id = cur_follow_cam;
            for(int i = 0; i < std::min(view_geoms.size(), (size_t) num_closest_views); ++i) {
                if(cam_id-int(view_geoms[i]->cam.cam_id) >= 0) {
                    closest.push_back(view_geoms[i]);  
                }
            }
            std::cout << std::endl << std::endl;

            current = closest;
            break;
        }        
    }
    return current;
}

const std::vector<SingleViewGeometry>& ViewSelector::get_all_static() const{
    return static_view_geoms;
}

const std::string& ViewSelector::get_current_single_view_name() const{
    static std::string empty = "empty";
    if(SingleViewGeometry::map.size() == 0) return empty;
    return current_single_view->first;
}
void ViewSelector::prev_single_view(){
    if(SingleViewGeometry::map.size() == 0) return;
    if(current_single_view-- == SingleViewGeometry::map.begin())
        current_single_view = --SingleViewGeometry::map.end();
}
void ViewSelector::next_single_view(){
    if(SingleViewGeometry::map.size() == 0) return;
    if(++current_single_view == SingleViewGeometry::map.end())
        current_single_view = SingleViewGeometry::map.begin();
}

void ViewSelector::control_gui(){
    // we assume Imgui::begin() and end() is called outside of this function!!
    ImGui::Separator();
    ImGui::Text("Set View Selection");
    if(ImGui::Button("All"))
        view_selector->set_mode(ViewSelector::ALL);
    ImGui::SameLine();
    if(ImGui::Button("Single view"))
        view_selector->set_mode(ViewSelector::SINGLE_VIEW);
    ImGui::SameLine();
    if(ImGui::Button("Closest"))
        view_selector->set_mode(ViewSelector::CLOSEST);
    ImGui::SameLine();
    if(ImGui::Button("Follow"))
        view_selector->set_mode(ViewSelector::FOLLOW);

    if(view_selector->mode == ViewSelector::SINGLE_VIEW){
        if(ImGui::Button("< pv"))
            view_selector->prev_single_view();
        ImGui::SameLine();
        ImGui::Text("%s", view_selector->get_current_single_view_name().c_str());
        ImGui::SameLine();
    if(ImGui::Button("pv >"))
            view_selector->next_single_view();
    }
    if(view_selector->mode == ViewSelector::CLOSEST){
        ImGui::DragInt("Num Views", &view_selector->num_closest_views, 1, 0, view_selector->view_geoms.size());
        ImGui::SliderInt("Patchification Level", &view_selector->mipmap_for_patchification, 1, 5);
        // ImGui::SameLine();
        ImGui::Text("Num Patches: %ix%i = %i", view_selector->current_num_patches_w, view_selector->current_num_patches_h, view_selector->current_num_patches);

        ImGui::Text("Coverage pooling");
        ImGui::Checkbox("Standard Mipmaping", &view_selector->mipmap);
        ImGui::Checkbox("cull_frustum", &view_selector->cull_frustum);
    }

    if(view_selector->mode == ViewSelector::FOLLOW){
        ImGui::DragInt("Num Views", &view_selector->num_closest_views, 1, 0, view_selector->view_geoms.size());
        ImGui::SliderInt("Current Follow Cam ID", &view_selector->cur_follow_cam, view_selector->min_cam_id, view_selector->max_cam_id);
    }
}


void ViewSelector::debug_gui(){
    ImGui::Begin("Selected Views"); 
    ImGui::Checkbox("Draw Selected Frusta", &view_selector->show_frusta);
    for(const auto& svg : view_selector->get_closest()){
        svg->svg_gui();
    }
    ImGui::End();
}