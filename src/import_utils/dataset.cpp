#include "colmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include "cv_utils.h"
#include "import_utils.h"

void Dataset::add_entry(const std::string& name, const CV_GL_Camera& cam, const std::string& image_path, const std::string& depth_path){
    entries.emplace(name, DatasetEntry{name, cam, image_path, depth_path});
}
        
void Dataset::remove_entry(const std::string& name){
    this->entries.erase(name);
}
        
void Dataset::export_as_colmap(std::string path){
// In COLMAP, calib-files export functions are in 
// colmap/src/base/reconstruction.cc
   
    { // write camera.txt
        std::cout << "Write " << path+"/cameras.txt" << std::endl;
        std::ofstream file(path+"/cameras.txt", std::ios::trunc);
        if (file.is_open()) {

            // Ensure that we don't loose any precision by storing in text.
            file.precision(17);

            file << "# Camera list with one line of data per camera:" << std::endl;
            file << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]" << std::endl;
            file << "# Number of cameras: " << entries.size() << std::endl;

            for (const auto& entry : entries) {
                const CV_GL_Camera& cam = entry.second.camera;
                std::ostringstream line;
                if(abs(cam.f_x-cam.f_y) > FLT_EPSILON) {
                    // std::cout << "WARNING: Mismachting f_x and f_y (diff = " << abs(cam.f_x-cam.f_y) << ") for cam" << cam.cam_id << ". Will be simplified to SIMPLE_PINHOLE with mean of f_x and f_y. See commented line in cameras.txt for actual values." << std::endl;
                    // line << "# ";
                    line << cam.cam_id << " ";
                    line << "PINHOLE ";
                    line << cam.w << " ";
                    line << cam.h << " ";
                    line << cam.f_x << " ";
                    line << cam.f_y << " ";
                    line << cam.c_x << " ";
                    line << cam.c_y << " ";

                    std::string line_string = line.str();
                    line_string = line_string.substr(0, line_string.size() - 1);

                    file << line_string << std::endl;
                }
                else {
                    line << cam.cam_id << " ";
                    line << "SIMPLE_PINHOLE ";
                    line << cam.w << " ";
                    line << cam.h << " ";
                    line << 0.5f*(cam.f_x+cam.f_y) << " ";
                    line << cam.c_x << " ";
                    line << cam.c_y << " ";

                    std::string line_string = line.str();
                    line_string = line_string.substr(0, line_string.size() - 1);

                    file << line_string << std::endl;
                }
            }
        } else {
            std::cout << "WARNING: could not write to " << path << std::endl;
        }
    }
    {// write images.txt
        std::cout << "Write " << path+"/images.txt" << std::endl;
        std::ofstream file(path+"/images.txt", std::ios::trunc);

        // Ensure that we don't loose any precision by storing in text.
        file.precision(17);

        file << "# Image list with two lines of data per image:" << std::endl;
        file << "#   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, "
                "NAME"
            << std::endl;
        file << "#   POINTS2D[] as (X, Y, POINT3D_ID)" << std::endl;
        file << "# Number of images: " << entries.size() << std::endl;

        uint32_t i = 0;
        for (const auto& entry : entries) {

            std::ostringstream line;
            std::string line_string;

            line << entry.second.camera.image_id << " ";

            glm::mat4 V = glm::inverse(COLMAP_to_GL_View)*entry.second.camera.get_gl_view();
            glm::quat qvec = glm::normalize(glm::toQuat(V));
            // QVEC (qw, qx, qy, qz)
            line << qvec.w << " ";
            line << qvec.x << " ";
            line << qvec.y << " ";
            line << qvec.z << " ";

            // TVEC
            line << V[3][0] << " ";
            line << V[3][1] << " ";
            line << V[3][2] << " ";

            line << entry.second.camera.cam_id << " ";
            line << entry.second.image_path.filename().string();

            file << line.str() << std::endl;

            // Leave points line blank... 
            file << std::endl;
            ++i;
        }
    }
}