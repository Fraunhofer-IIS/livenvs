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

CV_GL_Camera::CV_GL_Camera(const CV_GL_Camera& cam): CV_to_GL_View_Transform(cam.CV_to_GL_View_Transform){
    set_intrinsics(cam.image_id, cam.w, cam.h, cam.f_x, cam.f_y, cam.c_x, cam.c_y);
    set_extrinsics(cam.cam_id, cam.image_name, cam.rotation, cam.translation);
}

/// construct view matrix from R and trans as the last row of V
/// rotation rot and translation trans as given in the images.txt file
glm::mat4 rot_trans_to_viewmat(const glm::quat& rot, const glm::vec3& trans){
    glm::mat4 V = glm::toMat4(rot); // R mat
    V[3] = glm::vec4(trans.x, trans.y, trans.z, 1.f);
    return V;
}

/// construnct position of camera in world space, given as cam_pos = -R^T*trans
/// rotation rot and translation trans as given in the images.txt file
glm::vec3 rot_trans_to_campos(const glm::quat& rot, const glm::vec3& trans){
    glm::mat3 R = glm::toMat3(rot);
    glm::mat3 C = (-1.f*glm::transpose(R));
    glm::vec3 cam_pos = C*trans;
    return cam_pos;
}

void CV_GL_Camera::set_extrinsics(const uint32_t img_id, const std::string& img_name,
                             const glm::quat& rotation, const glm::vec3& translation){
    this->image_id = img_id;
    this->image_name = std::string(img_name);
    this->rotation = rotation;
    this->translation = translation;
}

void CV_GL_Camera::set_intrinsics(const uint32_t cam_id, const float w, const float h){
    this->cam_id = cam_id;
    this->w = w;
    this->h = h;
}

void CV_GL_Camera::set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f, const float c_x, const float c_y){
    this->cam_id = cam_id;
    this->w = w;
    this->h = h;
    this->f_x = f;
    this->f_y = f;
    this->c_x = c_x;
    this->c_y = c_y;
}

void CV_GL_Camera::set_intrinsics(const uint32_t cam_id, const float w, const float h, const float f_x, const float f_y, const float c_x, const float c_y){
    this->cam_id = cam_id;
    this->w = w;
    this->h = h;
    this->f_x = f_x;
    this->f_y = f_y;
    this->c_x = c_x;
    this->c_y = c_y;
}

glm::mat4 CV_GL_Camera::get_gl_view() const{
    const glm::mat4 V = rot_trans_to_viewmat(rotation, translation);
    return CV_to_GL_View_Transform * V;
}

glm::mat4 CV_GL_Camera::get_gl_proj(const float near, const float far) const {
    return gl_proj_from_cv_params(f_x, f_y, c_x, c_y, near, far, w, h); 
}

glm::vec3 CV_GL_Camera::get_cv_campos() const{
    return rot_trans_to_campos(rotation, translation);
}

glm::vec3 CV_GL_Camera::get_gl_campos() const{
    return glm::vec3(glm::inverse(get_gl_view())[3]);
}

float CV_GL_Camera::get_fov() const{
    return 0.5*f_y/h * float(180/M_PI);
}

glm::vec3 CV_GL_Camera::get_gl_viewdir() const{
    return glm::inverse(glm::mat3(get_gl_view()))*glm::vec3(0.f,0.f,-1.f);
}

glm::vec3 CV_GL_Camera::get_gl_updir() const{
    return glm::inverse(glm::mat3(get_gl_view()))*glm::vec3(0.f,1.f,0.f);
}

Dataset::Dataset(const std::string& mesh_path) : mesh_path(mesh_path) {};

Dataset::Dataset(std::function<std::string( uint32_t )> index_to_name, const std::string mesh_path) : 
    index_to_name(index_to_name), mesh_path(mesh_path) {};