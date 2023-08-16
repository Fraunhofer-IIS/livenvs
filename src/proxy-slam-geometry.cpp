#include "proxy-slam-geometry.h"
# include "import_utils/cv_utils.h"
# include "import_utils/import_utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>

// ------------------------------------------------
// prototypes

std::vector<Drawelement> ProxyObj::prototype;
std::shared_ptr<ProxyObj> proxyObj;

void init_proxy_model(const fs::path& model_path){

{ // load ProxyObj prototype
    proxyObj = std::make_shared<ProxyObj>();
    std::cout << fs::current_path() << std::endl;
    auto shader_norm = Shader("Proxy: default", "shader/pos+norm.vs", "shader/proxy_no_col.fs");
    auto shader_norm_tc = Shader("Proxy: write data", "shader/pos+norm+tc.vs", "shader/write_tc.fs");
    std::vector<Mesh> meshes = load_meshes_gpu(model_path, /* normalize */ false);
    ProxyObj::prototype.clear();
    ProxyObj::prototype.reserve(meshes.size());
    for (auto& mesh : meshes)
        ProxyObj::prototype.push_back(Drawelement(mesh->name, shader_norm_tc, mesh));             
	}
}

bool cap_resolution(uint32_t& w, uint32_t& h, const uint32_t max_resolution){
    const float scale = w > h ? 
        max_resolution/float(w) : 
        max_resolution/float(h);
    if (scale < 1.f){
        w = w * scale;
        h = h * scale;
        return true;
    }
    return false;
}


Framebuffer setup_framebuffer_for_img_scale(const std::string& img_name, const CV_GL_Camera& cam, uint32_t max_resolution){
    // read resolution
    // larger dimension will be scaled down to max_resolution pixels if bigger
    uint32_t w = cam.w;
    uint32_t h = cam.h;
    if(cap_resolution(w, h, max_resolution)){
        std::cout << "Scaled " << img_name << std::endl;
        std::cout << "Resolution: (" << cam.w << ", " << cam.h 
                    << ") --> (" << w << ", " << h << ")" << std::endl;
    }

    // create framebuffer and texture
    auto framebuf = Framebuffer(img_name, w, h);
    framebuf->attach_depthbuffer(Texture2D(img_name+"_depth", w, h, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT));
    framebuf->check();

    return framebuf;
}


Framebuffer setup_framebuffer_for_img_resolution(const std::string& img_name, uint32_t w, uint32_t h){
    // create framebuffer and texture
    auto framebuf = Framebuffer(img_name, w, h);
    framebuf->attach_depthbuffer(Texture2D(img_name+"_depth", w, h, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT));
    framebuf->check();

    return framebuf;
}

void render_depth_for_img(const std::string& img_name){
        // glEnable(GL_DEPTH_TEST);
        Framebuffer::find(img_name)->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render pointcloud
        //proxyObj->trafo = model;
        //proxyObj->trafo = glm::scale(glm::mat4(1.f), glm::vec3(100.f, 100.f, 100.f));
        const auto& shader = Shader::find("Proxy: write data");

        for(auto& elem : ProxyObj::prototype){
            elem->shader = shader;
        }
        shader->bind();
        shader->uniform("cam_pos_ws", current_camera()->pos);
        shader->uniform("model_normal", glm::transpose(glm::inverse(proxyObj->trafo)));
        shader->uniform("model_view", glm::transpose(glm::inverse(current_camera()->view*proxyObj->trafo)));
        proxyObj->draw();

        Framebuffer::find(img_name)->unbind();
        shader->unbind();
        for(auto& elem : ProxyObj::prototype){
            elem->shader = Shader::find("Proxy: default");
        }
}

void generate_proxy_data(const std::string& img_name, uint32_t w, uint32_t h){
        setup_framebuffer_for_img_resolution(img_name, w, h);
        // render wpos/ depth to fbos
        render_depth_for_img(img_name);
}


// ------------------------------------------------
// ProxyObj

ProxyObj::ProxyObj() {
    trafo = glm::mat4(1);
}

void ProxyObj::draw() {
    for (auto& elem : prototype) {
        elem->model = trafo;
        elem->bind();
        // setup_light(elem->shader);
        elem->draw(); //trafo);
        elem->unbind();
    }
}
void ProxyObj::draw_proxy() {
    proxyObj->draw();
}

ProxyObj::~ProxyObj() {}
