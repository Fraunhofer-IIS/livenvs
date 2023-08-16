#pragma once 

#include "camera.h"
#include "drawelement.h"

CPPGL_NAMESPACE_BEGIN

// ----------------------------------------------------
// visualization of camera frusta
namespace CameraVisualization {

extern bool lines;
extern float line_width;
extern float clamp_far;
extern glm::vec3 uniform_color;

extern bool display_all_cameras;
extern std::map<std::string, bool> active_cameras;


// excludes current camera
extern bool display_got_called;
void display_all_active();

// display single frustum
void display(const std::string& name);
void display(const Camera& camera);
void display(const glm::mat4& view, const glm::mat4& proj);
void display_view(const Texture2D& tex);

};


CPPGL_NAMESPACE_END
