#include "globals+gui.h"
#include <torchgl.h>
#include "single-view-geometry.h"
// #include "effect-simulator.h"
#include "encoded-cache.h"
#include "view-selector.h"
#include "rendering.h"

// extern std::shared_ptr<ViewSelector> view_selector;
extern std::shared_ptr<EncodedCache<SingleViewGeometry, InteropTexture2DArray>> enc_cache;


void globals_n_gui::gui_callback(){
    // using namespace gg;

    static bool show_svg_menu = false;
    if (ImGui::Begin("Project GUI", &show_project_gui)) {
        ImGui::Text("Memory Consumption");
        size_t free_mem, total_mem;
        cudaMemGetInfo(&free_mem, &total_mem);
        // ImGui::Text("GL Textures: %.2f GB, Cached Tensors: %.2f GB", 
        //     Texture2DImpl::accum_texture_storage*1e-9, enc_cache->used_memory*1e-9);
        ImGui::Text("Free Mem: %.2f GB of total %.2f GB",
            free_mem*1e-9, total_mem*1e-9);
        ImGui::Separator();

        ImGui::Text("Render & Aux Modes");
        static std::map<std::string, SingleViewGeometryImpl::DrawMode> labels_to_mode = {
            {"Points", SingleViewGeometryImpl::DrawMode::POINTS},
            {"Color Forward", SingleViewGeometryImpl::DrawMode::COL_FORWARD},
            {"Neural Forward", SingleViewGeometryImpl::DrawMode::N_FORWARD},
            {"Neural Deferred", SingleViewGeometryImpl::DrawMode::N_DEFERRED} };
        static std::string cur_label = "Color Forward";
        if (ImGui::BeginCombo("Render Mode", cur_label.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            for (const auto& [label, mode] : labels_to_mode)
            {
                bool is_selected = (cur_label == label); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(label.c_str(), is_selected)){
                    cur_label = label;
                    SingleViewGeometryImpl::set_drawmode(mode);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("Render Auxiliaries", &render_aux);
        
        ImGui::Separator();
        ImGui::Text("Current Camera: %s", current_camera()->name.c_str());
        glm::mat4 view = current_camera()->view;
        ImGui::Text("View: %s", glm::to_string(view).c_str());
        glm::mat4 proj = current_camera()->proj;
        ImGui::Text("Proj: %s", glm::to_string(proj).c_str());

        // view stuff
        ImGui::Separator();
        ImGui::Checkbox("Show SVG Menu", &show_svg_menu);
        ImGui::Separator();
        ViewSelector::control_gui();
        ImGui::Separator();
        EncodedCache<SingleViewGeometry, InteropTexture2DArray>::control_gui(*enc_cache);

        //
        ImGui::Separator();
        ImGui::Text("Warping");
        ImGui::SliderFloat("beta target", &beta_tgt[0], beta_tgt[1], beta_tgt[2], "%.6f");
        ImGui::SliderFloat("beta source", &beta_src[0], beta_src[1], beta_src[2], "%.6f");
        ImGui::Checkbox("Hole Filling", &hole_filling);
        ImGui::SliderFloat("Cut Off Depth", &cut_off_depth[0], cut_off_depth[1], cut_off_depth[2], "%.2f");
        ImGui::SliderFloat("Cut Off Depth for Deferred Depth", &cut_off_depth_deferred[0], cut_off_depth_deferred[1], cut_off_depth_deferred[2], "%.2f");
        ImGui::SliderFloat("Inf Depth for Deferred Background", &inf_depth_deferred, cut_off_depth_deferred[1], cut_off_depth_deferred[2], "%.2f");
        ImGui::Checkbox("Use background in deferred", &use_background_deferred);
        ImGui::Checkbox("Mask background", &mask_background);
        ImGui::Text("Use weights");
        ImGui::Checkbox("Vignette", &use_vignette_weight);
        ImGui::Checkbox("View Similarity", &use_view_weight);
        ImGui::Checkbox("Depth", &use_depth_weight);
        ImGui::SliderInt("Exponent", &weight_exponent, 1, 6);

        if(ImGui::Checkbox("Culling", &culling)){
            if (glIsEnabled(culling))
                glDisable(GL_CULL_FACE);
            else
                glEnable(GL_CULL_FACE);
        }

        // if(ImGui::Button("Toggle Datasets")) toggle_datasets = true;
        // if(play_demo){
        //     if(ImGui::Button("Pause Demo")) play_demo = false;
        // }else{
        //     if(ImGui::Button("Play Demo")) play_demo = true;
        // }
        // ImGui::SameLine();
        // ImGui::Checkbox("Record", &record_demo);
        // ImGui::SameLine();
        // ImGui::Checkbox("Demo cam", &use_demo_cam);

        // if(ImGui::Button("Play Animation")) {
        //     play_animation = true;
        //     Animation::find("animation")->play();
        // }
        // static std::vector<std::pair<glm::vec3, glm::quat>> keyframes;
        // if(ImGui::Button("Store Keyframes")) {
        //     glm::vec3 pos;
        //     glm::quat rot;
        //     current_camera()->store(pos, rot);

        //     keyframes.push_back({pos, rot});
        //     for(const auto& kf : keyframes)
        //         std::cout << glm::to_string(kf.first) << ", " << glm::to_string(kf.second) << std::endl;
        // }
        // if(ImGui::Button("Play Keyframes")) {
        //     static Animation anim("keyframes");
        //     anim->clear();
        //     anim->ms_between_nodes = 3000.f;
        //     for(const auto& kf : keyframes)
        //         anim->push_node(kf.first, kf.second);
        //     anim->play();
        // }
        // if(ImGui::Button("Write Animation")) render_anim = true;
        // ImGui::SliderInt("Bundle Index", (int*)&merge_depth_bundle_index, 0, 250);
        // if(ImGui::Button("Merge Depth")) merge_depth = true;
        // if(ImGui::Button("Render fused depths to tex")) render_depths_of_traj = true;
        // if(ImGui::Button("Render eval traj")) render_traj = true;
        // if(ImGui::Button("Write out fused depths")) write_depths_of_traj = true;

        // if constexpr (std::is_same<SingleViewGeometry,NamedHandle<EffectSimulatorGeometryImpl>>::value){
        //     ImGui::Separator();
        //     ImGui::Text("Simulate Effects");
        //     bool changed = false;    


        //     ImGui::Text("Camera Location Drift Intensity");
        //     if(ImGui::SliderFloat("Add Height according to fac*x", 
        //             &camera_location_drift_intensity[0], camera_location_drift_intensity[1],camera_location_drift_intensity[2], "%.4f"))
        //         changed = true;  
        //     ImGui::Text("Camera Jitter Intensities");
        //     if(ImGui::SliderFloat("Location [m]", 
        //             &camera_location_jitter_intensity[0], camera_location_jitter_intensity[1],camera_location_jitter_intensity[2], "%.4f"))
        //         changed = true;
              
        //     if(ImGui::SliderFloat("Rotation [rad]", &camera_rotation_jitter_intensity[0], camera_rotation_jitter_intensity[1],camera_rotation_jitter_intensity[2], "%.4f"))
        //         changed = true;
        //     if(ImGui::SliderFloat("Focal Length [px]", &camera_focal_length_jitter_intensity[0], camera_focal_length_jitter_intensity[1], camera_focal_length_jitter_intensity[2], "%.4f"))
        //         changed = true;
        //     if(ImGui::SliderFloat("Optical Center [px]", &camera_optical_center_jitter_intensity[0], camera_optical_center_jitter_intensity[1], camera_optical_center_jitter_intensity[2], "%.4f"))
        //         changed = true;


        //     ImGui::Text("Depth Map Artifacts Intensities");
        //     if(ImGui::SliderFloat("Noise [m]", &depth_map_noise_intensity[0], depth_map_noise_intensity[1], depth_map_noise_intensity[2], "%.4f"))
        //         changed = true;
        //     if(ImGui::SliderInt("Noise Period [px]", &depth_map_noise_period[0], depth_map_noise_period[1], depth_map_noise_period[2], "%.4f"))
        //         changed = true;
        //     if(ImGui::SliderFloat("Holes", &depth_map_holes_threshold[0], depth_map_holes_threshold[1], depth_map_holes_threshold[2], "%.4f"))
        //         changed = true;
        //     if(ImGui::SliderInt("Holes size", &depth_map_holes_size[0], depth_map_holes_size[1], depth_map_holes_size[2], "%.4f"))
        //         changed = true;
            

        //     if(changed)
        //         for(auto& [k,sg] : SingleViewGeometry::map) sg->update();
        // }
        
        // ImGui::Checkbox("Filtered Depthmaps", &use_filtered_depthmaps);

        ImGui::Separator();
        ImGui::Text("Nets");
        ImGui::Checkbox("Sync after Net", &sync_active);
        ImGui::SliderFloat("Feedback weight", &temporal_feedback_weight, 0.f, 1.f, "%.2f");

    }
    ImGui::End();

    if (show_svg_menu)
        ViewSelector::debug_gui();

    if(render_aux){
        auxiliaries_gui();
    }

}