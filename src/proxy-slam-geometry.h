#pragma once

#include <list>
#include <torchgl.h>


void init_proxy_model(const fs::path& model_path);
// remember to set current_camera before using this!
void generate_proxy_data(const std::string& img_name, uint32_t w, uint32_t h);

class ProxyObj {
public:
    ProxyObj();
    ~ProxyObj();
    
    static void draw_proxy();
    void draw();

    // data
    glm::mat4 trafo;
	static std::vector<Drawelement> prototype;
};
