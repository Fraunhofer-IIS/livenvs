#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "named_handle.h"

CPPGL_NAMESPACE_BEGIN

// ------------------------------------------
// Animation

class AnimationImpl {
public:
    AnimationImpl(const std::string& name);
    virtual ~AnimationImpl();

    // step animation and apply to CameraPtr::current() if running
    void update(float dt_ms);

    void clear();
    size_t length() const;

    void play();
    void pause();
    void toggle_pause();
    void stop();
    void reset();

    // add/modify camera path nodes
    size_t push_node(const glm::vec3& pos, const glm::quat& rot);
    void put_node(size_t i, const glm::vec3& pos, const glm::quat& rot);
    // add/modify data nodes along camera path
    size_t push_data(const std::string& name, const std::any& data);
    void put_data(size_t i, const std::string& name, const std::any& data);

    // evaluate
    glm::vec3 eval_pos() const;
    glm::quat eval_rot() const;    
    template <typename T> T eval_data(const std::string& name) const; // with interpolation (requires * operator with float)
    template <typename T> T lookup_data(const std::string& name) const; // without interpolation


    // TODO serialization

    // data
    const std::string name;
    float time;
    float ms_between_nodes;
    bool running;
    std::vector<std::pair<glm::vec3, glm::vec3>> camera_path;
    std::map<std::string, std::vector<std::any>> data_path;
};

using Animation = NamedHandle<AnimationImpl>;
template class _API NamedHandle<AnimationImpl>; //needed for Windows DLL export

// TODO move this to own module
Animation current_animation();
void make_animation_current(const Animation& anim);

CPPGL_NAMESPACE_END
