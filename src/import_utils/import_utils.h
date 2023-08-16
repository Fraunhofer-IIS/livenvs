#pragma once
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <vector>
#include <functional>

    /// Note: this is a camera in more in the GL sense, as it fuses in- and extrinsics into one object!
    /// Thus, the colmap <--> GL/ cppgl conversion, is more convenient to use.
    /// However, all axis and camera frame conventions are still in the CV sense until you call the get_gl_*() functions!! 
    struct CV_GL_Camera {
        // identifiers
        uint32_t cam_id;
        uint32_t image_id;
        std::string image_name;
        
        //intrinsics
        float w;
        float h;
        float f_x;
        float f_y;
        float c_x;
        float c_y;

        // extrinsics
        glm::quat rotation;
        glm::vec3 translation;
        // not static for easier inheritance and mixup of datasets!
        glm::mat4 CV_to_GL_View_Transform;

        // TODO add copy c-tor
        CV_GL_Camera() :CV_to_GL_View_Transform(glm::mat4(1.f)) {}
        CV_GL_Camera(const glm::mat4& CV_to_GL_View_Transform): CV_to_GL_View_Transform(CV_to_GL_View_Transform) {}
        virtual ~CV_GL_Camera(){}
        CV_GL_Camera(const CV_GL_Camera& cam);

        void set_extrinsics(const uint32_t img_id, const std::string& img_name,
                             const glm::quat& rotation, const glm::vec3& translation);
        void set_intrinsics(const uint32_t cam_id, const float w, const float h);
        void set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f, const float c_x, const float c_y);
        void set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f_x, const float f_y, const float c_x, const float c_y);
        
        glm::vec3 get_cv_campos() const;

        glm::mat4 get_gl_view() const;
        glm::vec3 get_gl_campos() const;
        glm::vec3 get_gl_viewdir() const;
        glm::vec3 get_gl_updir() const;
        glm::mat4 get_gl_proj(const float near, const float far) const;
        float get_fov() const;
    };

    
#include <filesystem>
namespace fs = std::filesystem;

    struct DatasetEntry {
        std::string name;
        CV_GL_Camera camera;
        fs::path image_path;
        fs::path depth_path; // might be empty
    };

    struct Dataset{
        Dataset(const std::string& mesh_path = "");
        Dataset(std::function<std::string( uint32_t )> index_to_name, const std::string mesh_path = "");
        std::string mesh_path; // might be empty
        std::map<std::string, DatasetEntry> entries;
        std::function<std::string( uint32_t )> index_to_name; // define a policy to map an index to an image name

        void add_entry(const std::string& name, const CV_GL_Camera& cam, const std::string& image_path, const std::string& depth_path);
        void remove_entry(const std::string& name);
        void export_as_colmap(std::string path);
    };


// void write_png(const std::string& filename, const std::vector<uint8_t>& pixels, uint32_t w, uint32_t h, uint32_t c);