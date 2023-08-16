#include "single-view-geometry.h"

#include <glm/gtx/string_cast.hpp>
#include <execution>

// #define STB_IMAGE_IMPLEMENTATION
#include "../external/torchgl/external/cppgl/src/stbi/stb_image.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"
// #include "../tiny_image_io.h"
#include "globals+gui.h"
extern globals_n_gui gg;

std::vector<Drawelement> SingleViewGeometryImpl::prototype_hq;
std::vector<Drawelement> SingleViewGeometryImpl::prototype_lq;
SingleViewGeometryImpl::DrawMode SingleViewGeometryImpl::mode = SingleViewGeometryImpl::DrawMode::COL_FORWARD;


static uint32_t linear_index(const uint32_t x, const uint32_t y, const uint32_t w){
    return y*w+x;
}

Mesh local_geom_mesh(const std::string& name, const uint32_t w, const uint32_t h){
        auto mesh = Mesh(name);
        // add bullshit positions (not needed)
        // mesh->add_vertex_buffer(GL_FLOAT, 3, w*h, std::vector<float>(w*h*3,0).data());

        // add texcoords.
        auto tex_coords = std::vector<glm::vec2>(w*h);
        for (uint32_t i = 0; i < w*h; ++i)
            tex_coords[i] = glm::vec2(float(i%w+0.5)/w, float(i/w+0.5)/h);
        mesh->add_vertex_buffer(GL_FLOAT, 2, w*h, tex_coords.data()); 
        
        // add indices
        uint32_t num_tris = 2*(w-1)*(h-1);
        uint32_t num_indices = 3*num_tris;
        auto indices = std::vector<uint32_t>();
        indices.reserve(num_indices);
        for (uint32_t y = 0; y < h-1; ++y){
            for (uint32_t x = 0; x < w-1; ++x){
                // tri 0
                indices.push_back(linear_index(x,   y,   w));
                indices.push_back(linear_index(x+1, y+1, w)); 
                indices.push_back(linear_index(x,   y+1, w)); 
                // tri 1
                indices.push_back(linear_index(x,   y,   w));
                indices.push_back(linear_index(x+1, y,   w)); 
                indices.push_back(linear_index(x+1, y+1, w)); 
            }
        }
        mesh->add_index_buffer(indices.size(), indices.data());
        SingleViewGeometryImpl::set_drawmode(SingleViewGeometryImpl::mode);
        return mesh;
}

void init_local_geom(){
    { 
        auto shader = Shader("Scene: localgeom", 
                    "shader/localgeom.vs", 
                    "shader/localgeom.fs");
        auto shader_refine_mesh = Shader("Scene: localgeom_refine", 
                    "shader/localgeom_refine_mesh.vs",
                    "shader/localgeom_refine_mesh.gs",
                    "shader/localgeom.fs");
        auto shader_fuse_feat = Shader("Scene: localgeom_fuse_feat", 
                    "shader/localgeom_refine_mesh.vs",
                    "shader/localgeom_refine_mesh.gs",
                    "shader/localgeom_fuse_feat.fs");
        auto shader_deferred = Shader("Scene: localgeom_deferred", 
                    "shader/localgeom_refine_mesh.vs",
                    "shader/localgeom_refine_mesh.gs",
                    "shader/localgeom_deferred.fs");

        SingleViewGeometryImpl::prototype_hq.clear();
        SingleViewGeometryImpl::prototype_hq.push_back(
            Drawelement("LocalGeometry HQ", shader, local_geom_mesh("LocalGeom HQ", gg.LOCAL_GEOM_HQ_WIDTH, gg.LOCAL_GEOM_HQ_HEIGHT)));  
                       
        SingleViewGeometryImpl::prototype_lq.clear();
        SingleViewGeometryImpl::prototype_lq.push_back(
            Drawelement("LocalGeometry LQ", shader, local_geom_mesh("LocalGeom LQ", gg.LOCAL_GEOM_LQ_WIDTH, gg.LOCAL_GEOM_LQ_HEIGHT)));  
                       
                   
	}
}


SingleViewGeometryImpl::SingleViewGeometryImpl(const std::string& name, const CV_GL_Camera& cam,
    const InteropTexture2D& depth_tex, const InteropTexture2D& color_tex, 
    const glm::mat4& inv_proj_view, const float min_d, const float max_d)
    :name(name), cam(cam), depth_texture(depth_tex), color_texture(color_tex), 
        inv_view_proj(inv_proj_view), min_d(min_d), max_d(max_d) {
        if(prototype_hq.empty()) init_local_geom();
}


SingleViewGeometryImpl::SingleViewGeometryImpl(const std::string& name)
    :name(name) {
        if(prototype_hq.empty()) init_local_geom();
}

void SingleViewGeometryImpl::set_drawmode(const DrawMode mode){
    SingleViewGeometryImpl::mode = mode;
    for (auto& elem : prototype_hq) {
        switch (mode) {
            case POINTS:
                elem->mesh->set_primitive_type(GL_POINTS);
                elem->shader = Shader::find("Scene: localgeom");
                break;
            case COL_FORWARD:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_refine");
                break;
            case N_FORWARD:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_fuse_feat");
                break;
            case N_DEFERRED:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_deferred");
                break;
            default:
                break;
        }
    }
    for (auto& elem : prototype_lq) {
        switch (mode) {
            case POINTS:
                elem->mesh->set_primitive_type(GL_POINTS);
                elem->shader = Shader::find("Scene: localgeom");
                break;
            case COL_FORWARD:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_refine");
                break;
            case N_FORWARD:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_fuse_feat");
                break;
            case N_DEFERRED:
                elem->mesh->set_primitive_type(GL_TRIANGLES);
                elem->shader = Shader::find("Scene: localgeom_deferred");
                break;
            default:
                break;
        }
    }
}

void SingleViewGeometryImpl::bind_elem(const Drawelement& elem) const{
    elem->bind();
    elem->shader->uniform("depth_tex", depth_texture->texture2D, 0);
    // elem->shader->uniform("color_tex", color_texture, 2);
    elem->shader->uniform("src_cam_pos_ws", cam.get_gl_campos());
    elem->shader->uniform("tgt_cam_pos_ws", current_camera()->pos);
    elem->shader->uniform("cut_off_depth", gg.cut_off_depth[0]);
    elem->shader->uniform("WIDTH", gg.WIDTH);
    elem->shader->uniform("HEIGHT", gg.HEIGHT);
    elem->shader->uniform("beta_tgt", gg.beta_tgt[0]);
    elem->shader->uniform("beta_src", gg.beta_src[0]);
    elem->shader->uniform("use_weights", glm::ivec4(gg.use_vignette_weight, gg.use_view_weight, gg.use_depth_weight, gg.weight_exponent));
}

void SingleViewGeometryImpl::draw(const bool HQ) const{
    if (hide) return;
    std::vector<Drawelement>& prototype = HQ ? prototype_hq : prototype_lq;
    for (auto& elem : prototype) {
        elem->model = inv_view_proj;
        bind_elem(elem);
        elem->shader->uniform("color_tex", color_texture->texture2D, 2);
        // setup_light(elem->shader);
        elem->draw(); //trafo);
        elem->unbind();
    }
}

// TODO: continue here. single view geometry actually contains everythin we need ....
// add adapter class? inherit from this class?

void SingleViewGeometryImpl::draw(const Texture2DArray& features, const bool HQ) const{
    if (hide) return;
    std::vector<Drawelement>& prototype = HQ ? prototype_hq : prototype_lq;
    for (auto& elem : prototype) {
        elem->model = inv_view_proj;
        
        bind_elem(elem);
        elem->shader->uniform("feat_tex", features, 2);
        // setup_light(elem->shader);
        elem->draw(); //trafo);
        elem->unbind();
    }
}

void SingleViewGeometryImpl::draw(const Shader& shader, const GLuint primitive_type, const float cut_off_depth, const bool HQ) const{
    if (hide) return;
    std::vector<Drawelement>& prototype = HQ ? prototype_hq : prototype_lq;
    for (auto& elem : prototype) {
        elem->shader = shader;
        elem->mesh->set_primitive_type(primitive_type);
        elem->model = inv_view_proj;
        
        bind_elem(elem);
        elem->shader->uniform("cut_off_depth", cut_off_depth); 
        elem->shader->uniform("color_tex", color_texture->texture2D, 2);
        elem->draw(); //trafo);
        elem->unbind();
    }

    set_drawmode(SingleViewGeometryImpl::mode);
}

void SingleViewGeometryImpl::update(const CV_GL_Camera& cam){
    this->cam = cam;
    inv_view_proj = glm::inverse(cam.get_gl_proj(gg.NEAR, gg.FAR)*cam.get_gl_view());
}

SingleViewGeometryImpl::~SingleViewGeometryImpl() {
    if(color_texture) InteropTexture2D::erase(color_texture->name);
    if(depth_texture) InteropTexture2D::erase(depth_texture->name);
    if(motion_texture) InteropTexture2D::erase(motion_texture->name);
    if(feature_texture_00_03) InteropTexture2D::erase(feature_texture_00_03->name);
    if(feature_texture_04_07) InteropTexture2D::erase(feature_texture_04_07->name);
    if(feature_texture_08_11) InteropTexture2D::erase(feature_texture_08_11->name);
    if(feature_texture_12_15) InteropTexture2D::erase(feature_texture_12_15->name);
}

void SingleViewGeometryImpl::svg_gui() const{
    ImGui::Separator();
    ImGui::Text("Name: %s", name.c_str());
    ImGui::Text("Image Name: %s", cam.image_name.c_str());
    glm::mat4 view = cam.get_gl_view();
    ImGui::Text("View: %s", glm::to_string(view).c_str());
    glm::mat4 proj = cam.get_gl_proj(gg.NEAR, gg.FAR);
    ImGui::Text("Proj: %s", glm::to_string(proj).c_str());
    ImGui::Text("InvProjView: %s", glm::to_string(inv_view_proj).c_str());

    float aspect_ratio = (gg.HEIGHT/(float)gg.WIDTH);
    if(color_texture)
        ImGui::Image((ImTextureID) color_texture->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(depth_texture)
        ImGui::Image((ImTextureID) depth_texture->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(motion_texture)
        ImGui::Image((ImTextureID) motion_texture->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(feature_texture_00_03)
        ImGui::Image((ImTextureID) feature_texture_00_03->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(feature_texture_04_07)
        ImGui::Image((ImTextureID) feature_texture_04_07->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(feature_texture_08_11)
        ImGui::Image((ImTextureID) feature_texture_08_11->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if(feature_texture_12_15)
        ImGui::Image((ImTextureID) feature_texture_12_15->texture2D->id, ImVec2(200, aspect_ratio*200), 
            ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));

}

void SingleViewGeometryImpl::show_frustum() const{
    glm::mat4 view = cam.get_gl_view();
    glm::mat4 proj = cam.get_gl_proj(gg.NEAR, gg.FAR);

    CameraVisualization::display(view, proj);
}