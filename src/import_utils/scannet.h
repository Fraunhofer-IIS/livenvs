#pragma once
#include <iostream>
#include "cv_utils.h"
#include "import_utils.h"

namespace scannet{

    /// Note: this is a camera in more in the GL sense, as it fuses in- and extrinsics into one object!
    /// Thus, the colmap <--> GL/ cppgl conversion, is more convenient to use.
    /// However, all axis and camera frame conventions are still in the CV sense until you call the get_gl_*() functions!! 
    struct Camera : CV_GL_Camera {
        // // identifiers
        // uint32_t cam_id;
        // uint32_t image_id;
        // std::string image_name;
        
        // //intrinsics
        // float w;
        // float h;
        // float f_x;
        // float f_y;
        // float c_x;
        // float c_y;

        // // extrinsics
        // glm::quat rotation;
        // glm::vec3 translation;
        // not static for easier inheritance and mixup of datasets!
        //glm::mat4 CV_to_GL_View_Transform;

        // TODO add copy c-tor

        Camera(const glm::mat4& CV_to_GL_View_Transform = COLMAP_to_GL_View):CV_GL_Camera(CV_to_GL_View_Transform){}
        Camera(const Camera& cam);
        virtual ~Camera(){}

        // void set_extrinsics(const uint32_t img_id, const std::string& img_name,
        //                      const glm::quat& rotation, const glm::vec3& translation);
        // void set_intrinsics(const uint32_t cam_id, const float w, const float h);
        // void set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f, const float c_x, const float c_y);
        // void set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f_x, const float f_y, const float c_x, const float c_y);
        

        // glm::mat4 get_gl_view() const;
        // glm::vec3 get_gl_campos() const;
        // glm::mat4 get_gl_proj(const float near, const float far) const;
    };

    Dataset load_dataset(const std::string& dir, const std::string& color_path, const std::string& depth_path, const std::string& color_file_ending=std::string(".jpg"), const std::string& depth_file_ending=std::string(".png"), const std::string& pose_format = "scannet");

    std::map<uint32_t, Camera> load_intrinsics(const std::string& cameras_txt_path);

} // namespace colmap