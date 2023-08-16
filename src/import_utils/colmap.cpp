#include "colmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include "cv_utils.h"

using namespace colmap;

Camera::Camera(const Camera& cam): CV_GL_Camera(cam){
    // set_intrinsics(cam.image_id, cam.w, cam.h, cam.f_x, cam.f_y, cam.c_x, cam.c_y);
}

struct camera_txt_entry {
    uint32_t cam_id;
    char cam_model[256];
    uint32_t width;
    uint32_t height;
    std::vector<double> params;
};

std::vector<camera_txt_entry> parse_camera_txt(const std::string& camera_txt_path) {

    std::vector<camera_txt_entry> entries;
    std::ifstream pos_file(camera_txt_path, std::ifstream::in);
    if (pos_file.is_open()) {
        std::cout << "parse " << camera_txt_path << "..." << std::endl;

        camera_txt_entry entry;

        std::string line_cam;
        std::string item;

        while (std::getline(pos_file, line_cam)) {
            // catch empty and comment lines
            if (line_cam.empty() || line_cam[0] == '#') 
                continue;
            
            std::stringstream line_stream(line_cam, std::ios::in);

            // id
            std::getline(line_stream, item, ' ');
            entry.cam_id = std::stoi(item);

            // camera model
            std::getline(line_stream, item, ' ');
            std::copy(item.c_str(), item.c_str()+item.size(), entry.cam_model);
            entry.cam_model[item.size()] = '\0';

            // width
            std::getline(line_stream, item, ' ');
            entry.width = std::stoi(item);

            // height
            std::getline(line_stream, item, ' ');
            entry.height = std::stoi(item);

            // further parameters (f_x, f_y, c_x, .......)
            entry.params.clear();
            while (!line_stream.eof()) {
                std::getline(line_stream, item, ' ');
                // std::cout << item << std::endl;
                if (!item.empty())
                    entry.params.push_back(std::stold(item));
            }

            entries.push_back(entry);
        }
    } else {
        std::cout << camera_txt_path << " was not opened!!" << std::endl;
    }
    std::cout << entries.size() << " entries loaded" << std::endl;
    return entries;
}

void print_camera_warning(const std::string& cam_model){
            std::cout   << "NOTE: " << cam_model << " is not representable by GL pinhole projections and will be simplified! " 
                        << "Please, undistort your images before importing them here!" << std::endl;
}

/// Params of camera models are thoroughly documented in colmap/src/base/camera_models.h 
std::map<uint32_t, Camera> colmap::load_intrinsics(const std::string& cameras_txt_path){

    std::vector<camera_txt_entry> entries = parse_camera_txt(cameras_txt_path);

    std::map<uint32_t, Camera> cameras;
    for (const auto& entry : entries){
        const std::string cam_model(entry.cam_model);
        // Directly usable pinhole models
        Camera cam;
        if          (cam_model == "SIMPLE_PINHOLE"){
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2]);
        } else if   (cam_model == "PINHOLE"){
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        // Note: follwing models will be simplified to pinhole models!
        } else if   (cam_model == "SIMPLE_RADIAL"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2]);
        } else if   (cam_model == "RADIAL"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2]);
        } else if   (cam_model == "OPENCV"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        } else if   (cam_model == "OPENCV_FISHEYE"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        } else if   (cam_model == "FULL_OPENCV"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        } else if   (cam_model == "FOV"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        } else if   (cam_model == "SIMPLE_RADIAL_FISHEYE"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2]);
        } else if   (cam_model == "RADIAL_FISHEYE"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2]);
        } else if   (cam_model == "THIN_PRISM_FISHEYE"){
            print_camera_warning(cam_model);
            cam.set_intrinsics(entry.cam_id, entry.width, entry.height, 
                                entry.params[0], entry.params[1], entry.params[2], entry.params[3]);
        // utter bullshit
        } else {
            std::cerr << cam_model << "is invalid! Please check out colmap/src/base/camera_models.h for valid camera models!" << std::endl;
            exit(-1);
        }
        cameras.emplace(entry.cam_id, cam);
    }

    return cameras;
}

struct image_txt_entry {
    uint32_t image_id;
    float q_w, q_x, q_y, q_z;
    float t_x, t_y, t_z;
    uint32_t cam_id;
    char image_filename[256];
};

std::vector<image_txt_entry> parse_image_txt(const std::string& images_txt_path) {

    std::vector<image_txt_entry> entries;
    std::ifstream pos_file(images_txt_path, std::ifstream::in);
    if (pos_file.is_open()) {
        std::string line_cam;
        std::string line_points;

        image_txt_entry entry;

        while (getline(pos_file, line_cam)) {
            // catch empty and comment lines
            if (line_cam.empty() || line_cam[0] == '#') 
                continue;

            // lines describing the image and lines that list feature point in an image are alternating
            if (!getline(pos_file, line_points))
                break;
            std::sscanf(line_cam.c_str(), "%u %f %f %f %f %f %f %f %u %s", 
                &entry.image_id, &entry.q_w, &entry.q_x, &entry.q_y, &entry.q_z, &entry.t_x, &entry.t_y, &entry.t_z, &entry.cam_id, entry.image_filename);

            entries.push_back(entry);
        }
    } else {
        std::cout << images_txt_path << " was not opened!!" << std::endl;
    }
    std::cout << entries.size() << " entries loaded" << std::endl;
    return entries;
}

Dataset colmap::load_dataset(const std::string& txt_dir, const std::string& img_dir, const std::string& depth_dir, const std::string& mesh_path, const std::string& color_file_ending, const std::string& depth_file_ending){
    // init
    Dataset dataset = Dataset([](uint32_t index)->std::string {return std::to_string(index);}, mesh_path);

    // parse images.txt and cameras.txt
    std::string cameras_txt_path = txt_dir + "cameras.txt";
    std::string images_txt_path = txt_dir + "images.txt";
    std::map<uint32_t, Camera> cameras = load_intrinsics(cameras_txt_path);
    std::vector<image_txt_entry> image_txt_entries = parse_image_txt(images_txt_path);

    for (const auto& entry : image_txt_entries){
        std::string image_filename(entry.image_filename);
        // use only stem as name
        image_filename = std::string(image_filename, 0, image_filename.find('.'));
        // create a copy of each real camera 
        // in order to duplicate the intrincs for each image camera
        Camera cam = cameras[entry.cam_id];
        // insert extrinsics of image camera
        glm::quat rot = glm::normalize(glm::quat(entry.q_w, entry.q_x, entry.q_y, entry.q_z));
        glm::vec3 trans = glm::vec3(entry.t_x, entry.t_y, entry.t_z);
        cam.set_extrinsics(entry.image_id, entry.image_filename, rot, trans);

        // full image path for file read
        std::string img_path = img_dir+image_filename+color_file_ending;

        // insert into dataset
        dataset.add_entry(image_filename, cam, 
            img_path, depth_dir.empty() ? "" : depth_dir+image_filename+depth_file_ending);

        // debug output
        // std::cout << image_filename << std::endl;
        // std::cout << "center: " << glm::to_string(cam_pos) << std::endl;
        // std::cout << "rotation: " << glm::to_string(rot) << std::endl;
        // std::cout << "R " << std::endl << glm::to_string(R) << std::endl;
        // std::cout << "C " << std::endl << glm::to_string(C) << std::endl;
        // std::cout << "v " << std::endl << glm::to_string(V) << std::endl;
        // std::cout << "inv v " << std::endl << glm::to_string(glm::inverse(v)) << std::endl;
    }

    return dataset;
}