#include "camera-trajectory-control.h"
#include "globals+gui.h"
extern globals_n_gui gg;

CameraTrajectoryControl::CameraTrajectoryControl(const Dataset& dataset, bool cycle):
    begin_frame_cam_it(dataset.entries.begin()),
    end_frame_cam_it(dataset.entries.end()),
    current_frame_cam_it(dataset.entries.begin()), 
    cycle(cycle) {}
    
const CV_GL_Camera& CameraTrajectoryControl::current() const {return current_frame_cam_it->second.camera;};
const CV_GL_Camera& CameraTrajectoryControl::begin() const {return begin_frame_cam_it->second.camera;};
const CV_GL_Camera& CameraTrajectoryControl::end() const {return end_frame_cam_it->second.camera;};

const CV_GL_Camera& CameraTrajectoryControl::move_forward(){
    ++current_frame_cam_it;
    if (cycle){
        if(current_frame_cam_it == end_frame_cam_it)
            current_frame_cam_it = begin_frame_cam_it;
    } 
    return current_frame_cam_it->second.camera;
}
const CV_GL_Camera& CameraTrajectoryControl::move_backward(){
    if (cycle){
        if(current_frame_cam_it == begin_frame_cam_it)
            current_frame_cam_it = end_frame_cam_it;
    }
    --current_frame_cam_it;
    return current_frame_cam_it->second.camera;

}

void CameraTrajectoryControl::reset(){
    current_frame_cam_it = begin_frame_cam_it;
}

void CameraTrajectoryControl::set_cppgl_camera_to_current(Camera& cppgl_camera){
    // instead of calling update() set view and proj directly
    // NOTE: gui display of teddy cam will be bullshit
    cppgl_camera->proj = current().get_gl_proj(gg.NEAR, gg.FAR);
    cppgl_camera->view = current().get_gl_view();
    cppgl_camera->pos = current().get_gl_campos();
    cppgl_camera->dir = current().get_gl_viewdir();
    cppgl_camera->up = current().get_gl_updir();
    cppgl_camera->fov_degree = current().get_fov();
}

void CameraTrajectoryControl::move_camera_forward(Camera& cppgl_camera){
    move_forward();
    set_cppgl_camera_to_current(cppgl_camera);
}

void CameraTrajectoryControl::move_camera_backward(Camera& cppgl_camera){
    move_backward();
    set_cppgl_camera_to_current(cppgl_camera);
}

void CameraTrajectoryControl::reset_camera(Camera& cppgl_camera){
    reset();
    set_cppgl_camera_to_current(cppgl_camera);
}