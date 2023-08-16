# pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>


/////////////////////////////////////////////////////////////////////////////////////////////
// get GL projectio matrix from intrinsics parameters
inline glm::mat4 gl_proj_from_cv_params(float f_x, float f_y, float c_x, float c_y, float near, float far, uint32_t w, uint32_t h){
    glm::mat4 proj = glm::mat4( 
        2*f_x/w,  0.0,    (w - 2*c_x)/w, 0.0,
        0.0,    2*f_y/h, (h - 2*c_y)/h, 0.0,
        0.0, 0.0, (-far - near) / (far - near), -2.0*far*near/(far-near),
        0.0, 0.0, -1.0, 0.0);
    return glm::transpose(proj);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//COLMAP view matrix convention: +X right, +Y down, +Z front; rhs 
//  GL_view_mat = COLMAP_to_GL_View*[R t]_colmap 
const glm::mat4 COLMAP_to_GL_View   = glm::mat4(1, 0, 0, 0,   0, -1, 0, 0,    0, 0, -1, 0,    0, 0, 0, 1); 


/////////////////////////////////////////////////////////////////////////////////////////////
// RealityCapture FBX view matrix convention: 
// inverse the GlobalTransform of the cameraNode and rotate by 90 deg, note that glm uses radians in newer versions!!! Very misleading in the documentation!
//  GL_view_mat = glm::rotate(glm::inverse(glm::make_mat4(cam->GetGlobalTransform()), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f)); 
// OR short
//  GL_view_mat = FBX_to_GL_View * glm::inverse(glm::make_mat4(cam->GetGlobalTransform()); 
const glm::mat4 FBX_to_GL_View  = glm::mat4(0, 0, -1, 0,     0, 1, 0, 0,     1, 0, 0, 0,     0, 0, 0, 1); // == rotateY(90)
// ------!!!!! NOTE: DO NOT USE:!!!!!!----------
    // const auto evalPos = camera->EvaluatePosition();
    // const auto evalTarget = camera->EvaluateLookAtPosition();
    // const auto evalUp = camera->EvaluateUpDirection(evalPos, evalTarget);
    // glm::vec3 upVector = glm::vec3(glm::make_vec4(evalUp.mData));
    // glm::vec3 targetVector = glm::vec3(glm::make_vec4(evalTarget.mData));
    // glm::vec3 posVector = glm::vec3(glm::make_vec4(evalPos.mData));
    // GL_view_mat = glm::lookAt(posVector, targetVector, upVector);
    //  or 
    // GL_view_mat = FBX_to_GL_View * glm::lookAt(posVector, targetVector, upVector);
// ----------!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!----------