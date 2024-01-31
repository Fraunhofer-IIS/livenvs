#include "preprocessor.h"
#include "torchgl.h"
#include "single-view-geometry.h"

#include <execution>

#include "globals+gui.h"
extern globals_n_gui gg;

// #define STB_IMAGE_IMPLEMENTATION
#include "../external/torchgl/external/cppgl/src/stbi/stb_image.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/torchgl/external/cppgl/src/stbi/stb_image_write.h"
// #include "../tiny_image_io.h"
// #include "../external/thirdparty/tinyimageloader/tiny_dng_loader.h"
#include "read_write_png_16_wrapper.h"
#include "limits.h"

Preprocessor::Preprocessor(){

}

// Texture2D scale_Texture2D(Texture2D& unscaled, const std::string& scaled_name, int w, int h, bool lin_interp){
//     // scale down
//         auto img_tensor = texture2D_to_tensor(unscaled).to(c10::ScalarType::Float).unsqueeze(0);
//         torch::ScalarType dt; 
//         int type_size;
//         get_dtype_from_GLenum(unscaled->type, dt, type_size);
//         img_tensor = lin_interp? 
//                         torch::upsample_bilinear2d(img_tensor, {h, w}, true).to(dt).squeeze(0)
//                 :       torch::upsample_nearest2d(img_tensor, {h, w}, true).to(dt).squeeze(0);
//         auto scaled = Texture2D(scaled_name, w, h, unscaled->internal_format, unscaled->format, unscaled->type);
//         tensor_to_texture2D(img_tensor, scaled, false);

//         return scaled;
// }


Texture2D scale_Texture2D(Texture2D& unscaled, const std::string& scaled_name, int w, int h, bool lin_interp){
    // scale down
    static auto shader = Shader("copy_tex_shader", "shader/copytex.vs", "shader/copytex.fs");
    Framebuffer fb = Framebuffer("tex_copy_fb", w, h);
    auto scaled = Texture2D(scaled_name, w, h, unscaled->internal_format, unscaled->format, unscaled->type);
    fb->attach_colorbuffer(scaled);
    fb->bind();
	shader->bind();
    shader->uniform("tex", unscaled, 0);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind(); 
    fb->unbind();

    Framebuffer::erase("tex_copy_fb");

    return scaled;
}



void Preprocessor::process_dataset_from_mesh(const Dataset& dataset, const uint32_t max_imgs, const uint32_t freq_imgs){
    init_proxy_model(dataset.mesh_path);

    // init cppgl cam
    auto processing_cam = Camera("processing_cam");    

    // TODO: XXX Y segfault without calling current->update initially?!?!?
    current_camera()->update();
    make_camera_current(processing_cam);

    
    uint32_t max_entries = 0;

    //iterate dataset
    std::cout << "Init textures and frambuffers for dataset imgs" << std::endl;
    CameraTrajectoryControl processing_traj = CameraTrajectoryControl(dataset, false);
    while(&processing_traj.current() != &processing_traj.end()){
        const uint32_t w = gg.LOCAL_GEOM_HQ_WIDTH, h = gg.LOCAL_GEOM_HQ_HEIGHT;
        //early out
        int id = max_entries++;
        if(max_entries > max_imgs || id % freq_imgs != 0) {
            processing_traj.move_forward();
            continue;
        }
        const std::string& img_name = std::string(processing_traj.current().image_name, 0, processing_traj.current().image_name.find('.'));

        const auto& img_cam  = processing_traj.current();
        processing_traj.set_cppgl_camera_to_current(processing_cam);        

        std::cout << "Process " << img_name << ":" << std::endl;
        std::cout << "Create depth and view dirs from proxy mesh...";
        generate_proxy_data(img_name, w, h);
        std::cout << " Done." << std::endl;

        // load RGB images to textures
        std::cout << "Load RGB image " << img_name << std::endl;
        std::cout << dataset.entries.count(img_name) << std::endl;
        std::cout << dataset.entries.begin()->first << std::endl;
        std::cout << dataset.entries.size() << std::endl;

        
        if (!fs::exists(dataset.entries.at(img_name).image_path))
            std::cout << dataset.entries.at(img_name).image_path << " does not exist!" << std::endl;
        auto unscaled_color = Texture2D(img_name+"_unscaled_col", dataset.entries.at(img_name).image_path);
        // scale down
        auto color = scale_Texture2D(unscaled_color, img_name+"_col", w, h, true);
        Texture2D::erase(unscaled_color->name);

        glm::mat4 inv_proj_view = glm::inverse( processing_cam->proj*processing_cam->view );
        SingleViewGeometry(img_name,
                img_cam,
                InteropTexture2D(img_name+"_depth", Texture2D::find(img_name+"_depth")),
                InteropTexture2D(img_name+"_col", Texture2D::find(img_name+"_col")), 
                inv_proj_view, gg.NEAR, gg.FAR
        );
        processing_traj.move_forward();

        std::cout << " Done." << std::endl;
    }    

}


static uint32_t linear_index(const uint32_t x, const uint32_t y, const uint32_t w){
    return y*w+x;
}

SingleViewGeometry Preprocessor::process_entry_from_depthmaps(const DatasetEntry& dset_entry){
            const uint32_t w = gg.LOCAL_GEOM_HQ_WIDTH, h = gg.LOCAL_GEOM_HQ_HEIGHT;
            const Camera& cppgl_cam = current_camera();


            static TimerQuery process_entry("Process Entry");
            static TimerQuery load_col("Load Color");
            static TimerQuery process_col("Process Color");
            static TimerQuery cast_depth("Cast Depth");
            static TimerQuery load_depth_new("Load Depth New");
            static TimerQuery process_depth("Process Depth");

            process_entry->begin();
                
            std::cout << "Process " << dset_entry.name << ":" << std::endl;
            // if( max_entries % freq_imgs == 1){
            //     continue;
            // }

            const std::string& img_name = dset_entry.name;
            const auto& img_cam = dset_entry.camera;

            // load RGB images to textures
            std::cout << "Load RGB image " << dset_entry.image_path << std::endl;
            load_col->begin();
            if (!fs::exists(dset_entry.image_path))
                std::cout << dset_entry.image_path << " does not exist!" << std::endl;
            auto unscaled_color = Texture2D(img_name+"_unscaled_col",dset_entry.image_path);
            load_col->end();
            std::cout << "Loaded " << dset_entry.image_path << std::endl;

            process_col->begin();
            // scale down
            auto color = scale_Texture2D(unscaled_color, img_name+"_col", w, h, true);
            Texture2D::erase(unscaled_color->name);
            process_col->end();

            // load RGB images to textures
            std::cout << "Load depth image " << dset_entry.depth_path << std::endl;
            load_depth_new->begin();
            uint32_t wi,hi;
            uint32_t channels = 1;
            if (!fs::exists(dset_entry.depth_path))
                std::cout << dset_entry.depth_path << " does not exist!" << std::endl;
            std::vector<uint16_t> data_16 = read_png_16(dset_entry.depth_path, wi, hi);
            load_depth_new->end();

            // write_png_16(out_file, image, 640, 480);


            cast_depth->begin();
            std::vector<float> data(wi*hi);

            static std::vector<uint32_t> indices;
            if (indices.size() == 0) {
                indices.reserve(data.size());
                for(uint32_t i = 0; i < data.size(); i++)
                    indices.push_back(i);
            }

            const glm::mat4 proj = img_cam.get_gl_proj(gg.NEAR, gg.FAR);

            // this seams not to be faster than the brute force method
            // std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),  
            //     [data_16, &data, channels, proj](uint32_t i) {
            //        // map to viewspace via depth shift of 1000
            //         data[i] = double(data_16[i*channels])/1000.0; // XXX depth shift
            //         // map to clip space
            //         // float z = proj[2][2]*(-data[i]) + proj[3][2];
            //         // float w = data[i];
            //         // z /= w;
            //         // data[i] = 0.5*z+0.5;


            //         glm::vec4 z = glm::vec4(0,0,-data[i],1.0);
            //         z = proj*z;
            //         z /= z.w;
            //         // map [-1,1] to [0,1]
            //         data[i] = 0.5*z.z+0.5;
            //         // if (data_16[i]!=0)
            //         //     std::cout << data_16[i] << ", ";
            //     }
            // );  

            float min_d = std::numeric_limits<float>::max();
            float max_d = std::numeric_limits<float>::min();

            // see depthpreprocessor2 https://github.com/darglein/saiga/blob/master/src/saiga/vision/util/DepthmapPreprocessor.cpp
            bool filter = true; 

            if (filter){

                // get median depth
                std::vector<uint16_t> data_16_copy;
                std::cout << "Copy "<< std::endl;
                // don't use 0s

                for(int i =0; i < wi*hi; ++i) {
                    if(data_16[i] > 1){
                        data_16_copy.push_back(data_16[i]);
                    }
                }
                
                // std::cout << "Depth Map orig linear size: " << data_16.size() << std::endl;
                // std::cout << "Depth Map linear size without 0s: " << data_16_copy.size() << std::endl;

                std::sort(data_16_copy.begin(), data_16_copy.end());
                // approximate median, not correct for uneven size
                double median = data_16_copy[data_16_copy.size() / 2]/1000.0;

                min_d = data_16_copy[0]/1000.0;
                max_d = data_16_copy[data_16_copy.size()-1]/1000.0;
                std::cout << "Valid depth range: Min " << min_d << ", Max " << max_d << std::endl;
                

                for(int i =0; i < wi*hi; ++i) {
                    // map to viewspace via depth shift of 1000
                    data[i] = double(data_16[i*channels])/1000.0; // XXX depth shift
                    // if (data[i] > 0) {// 0 is invalid
                    //     min_d = std::min(min_d, data[i]);
                    //     max_d = std::max(max_d, data[i]);
                    // }
                }

                    
                std::vector<float> data_filtered(wi*hi);
                float threshold = 0.03* median;
                int kernel_radius = 2;

                float sum = 0;
                std::cout << "Filter depth map." << std::endl;
                for (uint32_t y = kernel_radius; y < hi-(kernel_radius+1); ++y){
                    for (uint32_t x = kernel_radius; x < wi-(kernel_radius+1); ++x){
                            float cur = data[linear_index(x,y,wi)]; // XXX 
                            for (int i = -kernel_radius; i <= kernel_radius; i++){
                                for (int j = -kernel_radius; j <= kernel_radius; j++){
                                    if (i ==0 && j == 0) continue;                                    

                                    float neighbor = data[linear_index(x+i,y+j,wi)];
                                    // set to zero
                                    if (neighbor < 0.01){
                                        cur = 0.f;
                                    } else if ( std::abs(cur - neighbor) > threshold){
                                        cur = 0.f;
                                    }
                                }
                            }
                            sum+= cur;
                            data_filtered[linear_index(x,y,wi)] = cur;
                    }
                }

                for(int i =0; i < wi*hi; ++i) {      
                    // adapt to gl convention (missing depth is at far)
                    if (data_filtered[i] < 0.01)
                        data_filtered[i] = gg.FAR; 
                    
                    // map to clip space
                    float z = proj[2][2]*(-data_filtered[i]) + proj[3][2];
                    float w = data_filtered[i];
                    z /= w;
                    data[i] = 0.5*z+0.5;
                }

            } else {
                for(int i =0; i < wi*hi; ++i) {
                    // map to viewspace via depth shift of 1000
                    data[i] = double(data_16[i*channels])/1000.0; // XXX depth shift
                    if (data[i] > 0) {// 0 is invalid
                        min_d = std::min(min_d, data[i]);
                        max_d = std::max(max_d, data[i]);
                    }
                    
                    // adapt to gl convention (missing depth is at far)
                    if (data[i] < 0.01)
                        data[i] = gg.FAR; 
                    
                    // map to clip space
                    float z = proj[2][2]*(-data[i]) + proj[3][2];
                    float w = data[i];
                    z /= w;
                    data[i] = 0.5*z+0.5;
                    // map to clip space
                    // glm::vec4 z = glm::vec4(0,0,-data[i],1.0);
                    // z = proj*z;
                    // z /= z.w;
                    // // map [-1,1] to [0,1]
                    // data[i] = 0.5*z.z+0.5;
                    // if (data_16[i]!=0)
                    //     std::cout << data_16[i] << ", ";
                }
            }

            cast_depth->end();
            process_depth->begin();
            GLint internal_format = channels_to_float_format(1);
            GLint format = channels_to_format(1);
            GLenum type = GL_FLOAT;
            auto unscaled_depth = Texture2D(img_name+"_unscaled_depth", wi, hi, internal_format, format, type, data.data());
            // // scale down
            auto depth = scale_Texture2D(unscaled_depth, img_name+"_depth", w, h, false);
            Texture2D::erase(img_name+"_unscaled_depth");
            process_depth->end();

            glm::mat4 inv_proj_view = glm::inverse( img_cam.get_gl_proj(gg.NEAR, gg.FAR)*img_cam.get_gl_view() );
            

            process_entry->end();

            return SingleViewGeometry(img_name,
                    img_cam,
                    InteropTexture2D(img_name+"_depth", Texture2D::find(img_name+"_depth")),
                    InteropTexture2D(img_name+"_col", Texture2D::find(img_name+"_col")),  
                    inv_proj_view,
                    min_d, max_d
            );
}

Dataset Preprocessor::process_dataset_from_depthmaps(const Dataset& dataset, const uint32_t max_imgs, const uint32_t freq_imgs){
    // init_proxy_model(dataset.mesh_path);


    Dataset processed_dataset =  Dataset(dataset.index_to_name, dataset.mesh_path);

    std::atomic_int max_entries = 0;
    //iterate dataset
    std::cout << "Init textures and frambuffers for dataset imgs" << std::endl;
    std::cout << dataset.entries.size() << std::endl;
    // for(auto& cam_entry: dataset.camera ){
    std::for_each(std::execution::seq, dataset.entries.begin(), dataset.entries.end(),  
        [dataset, &processed_dataset, &max_entries, max_imgs, freq_imgs](const std::pair<std::string, DatasetEntry>& dset_entry) {
            //early out
            int id = max_entries++;
            if(max_entries > max_imgs || id % freq_imgs != 0) return;
            
            Preprocessor::process_entry_from_depthmaps(dset_entry.second);

            processed_dataset.entries[dset_entry.first] = dataset.entries.at(dset_entry.first); 
            std::cout << " Done." << std::endl;
        }
    );  
    return processed_dataset;
}

void Preprocessor::write_depthmap(const std::string& filename, const Texture2D& depth_tex, const glm::mat4& proj){

    std::vector<float> data;
    depth_tex->copy_from_gpu(data);
    const glm::mat4 inv_proj =  glm::inverse(proj);
        std::vector<uint16_t> data16(data.size());
        for(uint32_t i = 0; i < depth_tex->w*depth_tex->h; i++){
            // map to view space
            glm::vec4 z = glm::vec4(0,0,2.0*data[i]-1.0,1);

            // std::cout << glm::to_string(inv_proj) << std::endl;
            // std::cout << "data[i]  " << data[i] << std::endl;
            // std::cout << "z proj  " << z.z << std::endl;
            z = inv_proj*z;
            z.z = -z.z/z.w;
            // std::cout << "z view  " << z.z << std::endl;
            // std::cout << "w view  " << z.w << std::endl;
            data16[i] = uint16_t(double(z.z)*1000.0);
            // std::cout << "z uint16 * 1000  " << data16[i] << std::endl;
        }
        std::cout << "to  " << filename << std::endl;
    write_png_16(filename, data16, depth_tex->w,depth_tex->h);
}

void Preprocessor::save_merged_depthmaps(const std::string& dir){
    std::cout << "save preprocessed depthmaps" << std::endl;
    for(const auto& tex : merged_depthmaps){
        std::cout << "save " << tex.first << std::endl;
        const glm::mat4 proj = SingleViewGeometry::find(tex.first)->cam.get_gl_proj(gg.NEAR, gg.FAR);
        write_depthmap(dir + "/"+ tex.first +".png", tex.second, proj);
    }
}

std::string Preprocessor::process_dataset_bundle(const Dataset& dataset, uint32_t start_index, uint32_t end_index){
    std::cout << "Process bundle: " << start_index << ", " << end_index << std::endl;
    std::multimap<float, SingleViewGeometry> sorted_bundle;
    std::string best_name; 
    for(int frame_index = start_index; frame_index < end_index; ++frame_index) {
        // if (local_index > 20) continue;
        std::string name = dataset.index_to_name(frame_index);    
        std::cout << "Process: " << name << std::endl;
        if (SingleViewGeometry::valid(name)) {
            std::cout << "Does already exist... remove. " << std::endl;
            SingleViewGeometry::erase(name);
            if (merged_depthmaps.count(name)) merged_depthmaps.erase(name);
            if (Texture2D::valid(name+"_depth")) Texture2D::erase(name+"_depth");
            if (Texture2D::valid(name+"_merged_depth")) Texture2D::erase(name+"_merged_depth");
        }
        if (Texture2D::valid(name+"_depth")) {
            // Texture2D::erase(name+"_col");
            // Texture2D::erase(name+"_depth");
            // Texture2D::erase(name+"_merged_depth");
            // Texture2D::erase(name+"_motion");
            static bool first = true;
            if (first) std::cout << "endless loop?" << std::endl;
            first = false;
            return "";
        }
        SingleViewGeometry cur = Preprocessor::process_entry_from_depthmaps(dataset.entries.at(name));

        std::string last_name = dataset.index_to_name(frame_index-1);
        if (!SingleViewGeometry::valid(last_name)) {
            sorted_bundle.insert({100.f, cur});
            continue;
        }
        auto last = SingleViewGeometry::find(last_name);
        glm::mat from_cur_to_last = glm::inverse(last->inv_view_proj) * cur->inv_view_proj;

        static Shader motion_score = Shader("motion_score", "shader/motion_score.cs");
        Texture2D motion_tex = Texture2D(name+"_motion", gg.WIDTH, gg.HEIGHT, GL_R16F, GL_RED, GL_FLOAT, (void*)0, true);
        cur->motion_texture = InteropTexture2D(name+"_motion", motion_tex);
        motion_score->bind();
        motion_score->uniform("WIDTH", gg.WIDTH);
        motion_score->uniform("HEIGHT", gg.HEIGHT);
        motion_score->uniform("depth_img_last", last->depth_texture->texture2D, 0);
        motion_score->uniform("depth_img_cur", cur->depth_texture->texture2D, 1);
        motion_score->uniform("from_cur_to_last_mat", from_cur_to_last);
        motion_tex->bind_image(0, GL_READ_WRITE, GL_R16F);
        

        motion_score->dispatch_compute(gg.WIDTH, gg.HEIGHT);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        motion_score->unbind();

        motion_tex->bind(0);
        glGenerateMipmap(GL_TEXTURE_2D); 
        motion_tex->unbind();

        int level = int(std::log2(std::max(gg.WIDTH, gg.HEIGHT)));
        auto mean_motion_score = std::vector<float>();
        motion_tex->copy_from_gpu(mean_motion_score, level); 
        std::cout << "score" << std::endl;
        for(const auto& score : mean_motion_score) std::cout << score << std::endl;
        std::cout << " Done." << std::endl;

        sorted_bundle.insert({mean_motion_score[0], cur});
        scores[name] = mean_motion_score[0];
    }

    // Keep best view
    auto it = sorted_bundle.begin();
    best_name = it->second->name;
    std::cout << "keep." << std::endl;
    std::cout << best_name << "; Score: " << it->first << std::endl; 
    auto best = SingleViewGeometry::find(best_name);

    {
        // merge depth of best views
        // TODO 
        static Shader merge_depth_shader = Shader("merge_depth", "shader/merge_depth.cs");
        Texture2D merged_depth = Texture2D(best_name+"_merged_depth", gg.WIDTH, gg.HEIGHT, GL_R32F, GL_RED, GL_FLOAT);
        merge_depth_shader->bind();
        glm::mat4 merge_inv_mat = best->inv_view_proj;
        for (uint32_t j = 0; j < 4; j++){
            if (SingleViewGeometry::valid(it->second->name)){
                merge_depth_shader->uniform("depth_tex_"+std::to_string(j), SingleViewGeometry::find(it->second->name)->depth_texture->texture2D, j);

                if (j != 0) {
                    glm::mat from_merge_to_j = glm::inverse(SingleViewGeometry::find(it->second->name)->inv_view_proj) *merge_inv_mat;
                    merge_depth_shader->uniform("from_merged_to_"+std::to_string(j)+"_mat", from_merge_to_j);
                }
                it++;
            } else {
                std::cout << it->second->name << " is invalid... " << std::endl;
                j--;
            }
        }
        merged_depth->bind_image(4, GL_READ_WRITE, GL_R32F);
        merge_depth_shader->uniform("WIDTH", gg.WIDTH);
        merge_depth_shader->uniform("HEIGHT", gg.HEIGHT);

        merge_depth_shader->dispatch_compute(gg.WIDTH, gg.HEIGHT);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        merge_depth_shader->unbind();

        // update depth
        // best->depth_texture = merged_depth;
        merged_depthmaps[best_name] = merged_depth;
    }

    it = sorted_bundle.begin();
    it++;
    // remove others
    uint32_t erased_from_bundle = 0;
    for(; it != sorted_bundle.end(); it++){
        if (SingleViewGeometry::valid(it->second->name)){
            SingleViewGeometry::erase(it->second->name);
            erased_from_bundle++;
        }
        else std::cout << it->second->name << " not an SG!" << std::endl;
    }

    return best_name;
}

Dataset Preprocessor::process_dataset_from_depthmap_find_keyframes(const Dataset& dataset, int arg_max_images, int bundle_size){
    // init_proxy_model(dataset.mesh_path);

    std::atomic_int max_entries = 0;
    //iterate dataset
    std::cout << "Init textures and frambuffers for dataset imgs" << std::endl;
    std::cout << dataset.entries.size() << std::endl;
    // for(auto& cam_entry: dataset.camera ){
    
    Dataset processed_dataset =  Dataset(dataset.index_to_name, dataset.mesh_path);

    int frame_index = 0;
    for(int i = 0; i < dataset.entries.size() / bundle_size; ++i){
        frame_index = i * bundle_size;
        if (frame_index > arg_max_images) break;
        std::string best_name = process_dataset_bundle(dataset, frame_index, frame_index+bundle_size);
        processed_dataset.entries[best_name] = dataset.entries.at(best_name); 
        

        // if (i > 10) break;
        // return processed_dataset;
    }
    // std::cout << " processd " << frame_index << std::endl;
//     std::cout << "erased: " << erased  << std::endl;
    // std::cout << "sg map size " << SingleViewGeometry::map.size() << std::endl;
    // std::cout << " processed dataset size " << processed_dataset.entries.size() << std::endl;
    // std::cout << "kept: " << std::endl;
    // for (const auto& k : kept) std::cout << k << ", ";
    // std::cout << std::endl;
    // remove first?

    // std::cout << "number of keyframes: " << processed_dataset.entries.size() << std::endl;

    // processed_dataset.export_as_colmap(".");
    return processed_dataset;
}
