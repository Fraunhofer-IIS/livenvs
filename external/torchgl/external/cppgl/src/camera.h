#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "named_handle.h"

#undef far
#undef near

CPPGL_NAMESPACE_BEGIN

// ----------------------------------------------------
// Camera

class CameraImpl {
public:
    CameraImpl(const std::string& name);
    virtual ~CameraImpl();

    void update();
    void update_proj();

    // move
    void forward(float by);
    void backward(float by);
    void leftward(float by);
    void rightward(float by);
    void upward(float by);
    void downward(float by);

    // rotate
    void yaw(float angle);
    void pitch(float angle);
    void roll(float angle);

    // load/store
    void store(glm::vec3& pos, glm::quat& rot) const;
    void load(const glm::vec3& pos, const glm::quat& rot);
    void from_lookat(const glm::vec3& pos, const glm::vec3& lookat, const glm::vec3& up = glm::vec3(0, 1, 0));

    // compute aspect ratio from current viewport
    static float aspect_ratio();

    // default camera keyboard/mouse handler for basic movement
    static float default_camera_movement_speed;
    static bool default_input_handler(double dt_ms);

    // data
    const std::string name;
    glm::vec3 pos, dir, up;             // camera coordinate system
    float fov_degree;
    float near, far;        // perspective projection
    float left, right, bottom, top;     // orthographic projection
    bool perspective;                   // switch between perspective and orthographic (default: perspective)
    bool skewed;                        // switcg between normal perspective and skewed frustum (default: normal)
    bool fix_up_vector;                 // keep up vector fixed to avoid camera drift
    glm::mat4 view, view_normal, proj;  // camera matrices (computed via a call update())
};

using Camera = NamedHandle<CameraImpl>;
template class _API NamedHandle<CameraImpl>; //needed for Windows DLL export

// TODO move to CameraImpl::current()
Camera current_camera();
void make_camera_current(const Camera& cam);

CPPGL_NAMESPACE_END
