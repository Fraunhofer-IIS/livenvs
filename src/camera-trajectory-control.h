# pragma once

# include "torchgl.h"
# include "import_utils/cv_utils.h"
# include "import_utils/import_utils.h"

using trajectory_iterator = std::map<std::string, DatasetEntry>::const_iterator;

struct CameraTrajectoryControl {
    CameraTrajectoryControl(const Dataset& dataset, bool cycle);
    
    bool cycle;
    trajectory_iterator current_frame_cam_it;
    const trajectory_iterator begin_frame_cam_it;
    const trajectory_iterator end_frame_cam_it;


    const CV_GL_Camera& current() const;
    const CV_GL_Camera& begin() const;
    const CV_GL_Camera& end() const;
    
    const CV_GL_Camera& move_forward();
    const CV_GL_Camera& move_backward();
    void reset();

    void set_cppgl_camera_to_current(Camera& cppgl_camera);
    void move_camera_forward(Camera& cppgl_camera);
    void move_camera_backward(Camera& cppgl_camera);
    void reset_camera(Camera& cppgl_camera);


};