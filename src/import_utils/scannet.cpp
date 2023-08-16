#include "scannet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include "cv_utils.h"

#include <filesystem>
namespace fs = std::filesystem;
using namespace scannet;

Camera::Camera(const Camera& cam): CV_GL_Camera(cam){
    // set_intrinsics(cam.image_id, cam.w, cam.h, cam.f_x, cam.f_y, cam.c_x, cam.c_y);
}

struct image_txt_entry {
    uint32_t image_id;
    float q_w, q_x, q_y, q_z;
    float t_x, t_y, t_z;
    uint32_t cam_id;
    char image_filename[256];
};

static std::tuple<float, float> load_resolution_from_txt(const std::string& res_file){
    std::ifstream resolution_file(res_file, std::ifstream::in);
    if (resolution_file.is_open()) {
        std::cout << "parse " << res_file << "..." << std::endl;

        std::string line;
        std::string item;
        
        std::vector<int> res;

        uint32_t i = 0;
        while (std::getline(resolution_file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;
            
            std::stringstream line_stream(line, std::ios::in);

            while(std::getline(line_stream, item, ' ')){
                res.push_back(std::stof(item));
                // std::cout << std::stof(item) << ", ";
                i++;
            }
            // std::cout << std::endl;
        }
        // std::cout << glm::to_string(mat) << std::endl;
        resolution_file.close();
        if (res.size() != 2 ){
            std::cout << res_file << "does not contain 2 values" << std::endl;
            return std::tuple<float, float>(0,0);
        }
        return std::tuple<float, float>(res[0], res[1]);
        
    } else {
        std::cout << "could not opoen " << res_file << std::endl;
        return std::tuple<float, float>(0,0);
    }

}

static glm::mat4 load_mat_from_txt(const std::string& mat_file){
    std::ifstream intrinsics_file(mat_file, std::ifstream::in);
    if (intrinsics_file.is_open()) {
        std::cout << "parse " << mat_file << "..." << std::endl;

        std::string line;
        std::string item;
        
        glm::mat4 mat;

        uint32_t i = 0;
        while (std::getline(intrinsics_file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;
            
            std::stringstream line_stream(line, std::ios::in);

            while(std::getline(line_stream, item, ' ')){
                mat[i%4][i/4] = std::stof(item);
                // std::cout << std::stof(item) << ", ";
                i++;
            }
            // std::cout << std::endl;
        }
        // std::cout << glm::to_string(mat) << std::endl;
        intrinsics_file.close();
        return mat;
        
    } else {
        std::cout << "could not opoen " << mat_file << std::endl;
        return glm::mat4();
    }

}


static std::vector<glm::mat4> load_all_poses_from_adop_txt(const std::string& posefile){
    std::vector<glm::mat4> poses;
    std::ifstream extrinsics_file(posefile, std::ifstream::in);
    if (extrinsics_file.is_open()) {
        std::cout << "parse " << posefile << "..." << std::endl;

        std::string line;
        std::string item;
        
        glm::mat4 mat;

        uint32_t i = 0;
        while (std::getline(extrinsics_file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;
            
            float qx, qy, qz, qw;
            float x, y, z;

            std::sscanf(line.c_str(), "%f %f %f %f %f %f %f", 
                &qx, &qy, &qz, &qw, &x, &y, &z);

            glm::quat rot = glm::quat(qw, qx, qy, qz);
            glm::mat4 pose = glm::toMat4(rot);
            pose[3] = glm::vec4(x,y,z,1);

            poses.push_back(pose);
        }
        
        // std::cout << glm::to_string(mat) << std::endl;
        extrinsics_file.close();        
    } else {
        std::cout << "could not opoen " << posefile << std::endl;
    }
    return poses;
}

static std::map<std::string, std::string> load_associations_from_txt(const std::string& assfile){
    std::map<std::string, std::string> asses;
    std::ifstream associations_file(assfile, std::ifstream::in);
    if (associations_file.is_open()) {
        std::cout << "parse " << assfile << "..." << std::endl;

        std::string line;
        std::string item;
        
        uint32_t i = 0;
        while (std::getline(associations_file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;
            
            char index[512];
            char t[512];

            // timestamp tx ty tz qx qy qz qw
            std::sscanf(line.c_str(), "%s %s", 
                index, t);

            double td = std::stod(t);
            char t2[512];
            sprintf(t2,"%.5lf", td);

            asses[t2] = std::string(index);
        }
        
        // std::cout << glm::to_string(mat) << std::endl;
        associations_file.close();        
    } else {
        std::cout << "could not opoen " << assfile << std::endl;
    }
    return asses;
}

static std::map<std::string, glm::mat4> load_all_poses_from_tum_txt(const std::string& posefile){
    std::map<std::string, glm::mat4> poses;
    std::ifstream extrinsics_file(posefile, std::ifstream::in);
    if (extrinsics_file.is_open()) {
        std::cout << "parse " << posefile << "..." << std::endl;

        std::string line;
        std::string item;
        
        glm::mat4 mat;

        uint32_t i = 0;
        while (std::getline(extrinsics_file, line)) {
            // catch empty and comment lines
            if (line.empty() || line[0] == '#') 
                continue;
            
            char t[512];
            float qx, qy, qz, qw;
            float x, y, z;

            // timestamp tx ty tz qx qy qz qw
            std::sscanf(line.c_str(), "%s %f %f %f %f %f %f %f", 
                t, &x, &y, &z, &qx, &qy, &qz, &qw);

            glm::quat rot = glm::quat(qw, qx, qy, qz);
            glm::mat4 pose = glm::toMat4(rot);
            pose[3] = glm::vec4(x,y,z,1);
            
            double td = std::stod(t);
            char t2[512];
            sprintf(t2,"%.5lf", td);

            poses[t2] = pose;
        }
        
        // std::cout << glm::to_string(mat) << std::endl;
        extrinsics_file.close();        
    } else {
        std::cout << "could not opoen " << posefile << std::endl;
    }
    return poses;
}

Dataset scannet::load_dataset(const std::string& dir, const std::string& color_path, const std::string& depth_path, const std::string& color_file_ending, const std::string& depth_file_ending, const std::string& pose_format){
    // init
    Dataset dataset = Dataset(  
                            [](uint32_t index)->std::string { 
                                std::string name = std::to_string(index);
                                name = std::string(std::max(0, 4 - int(name.length())), '0') + name;
                                return name;
                            });

    // parse images.txt and cameras.txt
    const std::string intrinsics_path = dir + "intrinsic/";
    const std::string extrinsics_path = dir + "pose/";
    // load intrinsics

    auto [w , h] = load_resolution_from_txt(intrinsics_path+"/resolution.txt"); // TODO XXX
    // if (w == 0 || h == 0)
    //     [w , h] = load_resolution_from_txt(intrinsics_path+"/resolution_depth.txt"); // TODO XXX
    glm::mat4 intrinsics_depth = load_mat_from_txt(intrinsics_path+"/intrinsic_depth.txt");
    //TODO we have different intrinsics? how to handle??
    double f_x = intrinsics_depth[0][0]; //*(w/2.0);
    double f_y = intrinsics_depth[1][1]; //*(h/2.0);
    double c_x = ((intrinsics_depth[2][0])); //*w)-w)/(-2.0);
    double c_y = ((intrinsics_depth[2][1])); //*h)-h)/(-2.0);
    Camera cam_depth;
    cam_depth.set_intrinsics(1000000, w, h, f_x, f_y, c_x, c_y);
    std::cout << "depth in: " << glm::to_string(cam_depth.get_gl_proj(0.01, 1000.0)) << std::endl;
    
    auto [w_col , h_col] = load_resolution_from_txt(intrinsics_path+"/resolution.txt"); // TODO XXX
    glm::mat4 intrinsics_color = load_mat_from_txt(intrinsics_path+"/intrinsic_color.txt");
    double f_x_col = intrinsics_color[0][0]; //*(w/2.0);
    double f_y_col = intrinsics_color[1][1]; //*(h/2.0);
    double c_x_col = ((intrinsics_color[2][0])); //*w)-w)/(-2.0);
    double c_y_col = ((intrinsics_color[2][1])); //*h)-h)/(-2.0);
    Camera cam_col;
    cam_col.set_intrinsics(1000001, w_col, h_col, f_x_col, f_y_col, c_x_col, c_y_col);
    std::cout << "depth in: " << glm::to_string(cam_col.get_gl_proj(0.01, 1000.0)) << std::endl;

    // exit(0);

    // load poses
    std::vector<glm::mat4> cam_poses;
    if (pose_format == "adop") {// from ADOP
        std::cout << "load ADOP poses" << std::endl;
        std::string pose_file = dir+"poses_qxyzw_txyz.txt";
        cam_poses =  load_all_poses_from_adop_txt(pose_file);
    } else if(pose_format == "tum" || pose_format == "tum_full"){
        std::cout << "load TUM poses" << std::endl;
        std::string pose_file = pose_format == "tum" ? 
                    dir+"KeyframeTrajectory.txt" : dir+"CameraTrajectory.txt";
        std::string ass_file = dir+"associations.txt";
        auto asses = load_associations_from_txt(ass_file); 
        auto cam_poses_unsorted =  load_all_poses_from_tum_txt(pose_file);
        // XXX Note that we add leading zeros for the map but the image name obviiously is without...
        // build camera entries
        for(const auto& cp : cam_poses_unsorted){
            // std::cout << cp.first << std::endl;
            if (asses.count(cp.first) == 0)
                std::cout << cp.first << " not in ass file!" << std::endl;
            if (cam_poses_unsorted.count(cp.first) == 0)
                std::cout << cp.first << " not in pose file!" << std::endl;

            const std::string img_name = asses.at(cp.first);
            const glm::mat4 pose = cam_poses_unsorted.at(cp.first); 
            // std::cout << img_name << std::endl;
            const uint32_t img_index = std::stoi(img_name);

            Camera cam;
            
            cam.set_intrinsics(img_index, w, h, f_x, f_y, c_x, c_y);
            // load extrinsics
            // std::cout << glm::to_string(pose) << std::endl;
            glm::mat4 extrinsics_mat = glm::inverse(pose);
            cam.set_extrinsics(img_index, dataset.index_to_name(img_index), glm::quat_cast(glm::mat3(extrinsics_mat)), glm::vec3(extrinsics_mat[3]));
            // cam.set_extrinsics(img_index, img_name, glm::quat_cast(glm::mat3(1)), glm::vec3(0));

            // insert into dataset
            dataset.add_entry(dataset.index_to_name(img_index), cam, color_path+img_name+color_file_ending, depth_path+img_name+depth_file_ending);
            
        }
        return dataset;
        
    } else if(pose_format == "scannet") {// from scannet
        std::cout << "load SCANNET poses" << std::endl;
        uint32_t img_index = 0; 
        std::string img_name = std::to_string(img_index++);
        fs::path extrinsics_txt = fs::path(extrinsics_path+img_name+".txt");
        while( fs::exists(extrinsics_txt) ){
            // load extrinsics
            cam_poses.push_back(load_mat_from_txt(extrinsics_txt));

            // iterate
            img_name = std::to_string(img_index++);
            extrinsics_txt = fs::path(extrinsics_path+img_name+".txt");
            std::cout << extrinsics_txt << std::endl;
        }
    
    }

    // XXX Note that we add leading zeros for the map but the image name obviiously is without...
    // build camera entries
    for(uint32_t img_index = 0; img_index < cam_poses.size(); ++img_index){
        Camera cam;
        std::string img_name = pose_format != "scannet"? 
                                dataset.index_to_name(img_index) :
                                std::to_string(img_index);
        
        cam.set_intrinsics(img_index, w, h, f_x, f_y, c_x, c_y);
        // load extrinsics
        // std::cout << glm::to_string(cam_poses[img_index]) << std::endl;
        glm::mat4 extrinsics_mat = glm::inverse(cam_poses[img_index]);
        cam.set_extrinsics(img_index, dataset.index_to_name(img_index), glm::quat_cast(glm::mat3(extrinsics_mat)), glm::vec3(extrinsics_mat[3]));
        // cam.set_extrinsics(img_index, img_name, glm::quat_cast(glm::mat3(1)), glm::vec3(0));

        // insert into dataset
        dataset.add_entry(dataset.index_to_name(img_index), cam, color_path+img_name+color_file_ending, depth_path+img_name+depth_file_ending);
        
    }
    
    return dataset;
}