#pragma once

#include "torchgl.h"

# include "camera.h"
# include "import_utils/cv_utils.h"
# include "import_utils/import_utils.h"

#include "camera-trajectory-control.h"
#include "proxy-slam-geometry.h"

class SingleViewGeometryImpl {
public:
    enum DrawMode {
        POINTS,
        COL_FORWARD, 
        N_FORWARD, 
        N_DEFERRED 
    };

    SingleViewGeometryImpl(const std::string& name);
    SingleViewGeometryImpl(const std::string& name, const CV_GL_Camera& cam, 
        const InteropTexture2D& depth_tex, const InteropTexture2D& color_tex, const glm::mat4& inv_proj_view, const float min_d, const float max_d );
    ~SingleViewGeometryImpl();

    void bind_elem(const Drawelement& elem) const;
    void update(); // currently unused
    void update(const CV_GL_Camera& cam);
    void draw(const bool HQ = true) const;
    void draw(const Texture2DArray& features, const bool HQ = true) const;
    void draw(const Shader& shader, const GLuint primitive_type, const float cut_off_depth, const bool HQ = true) const;
    
    void svg_gui() const;
    void show_frustum() const;

    const std::string name;

    //interaction
    static DrawMode mode;
    static void set_drawmode(const DrawMode mode);

    //view
    CV_GL_Camera cam;

    //textures
    InteropTexture2D depth_texture;
    InteropTexture2D motion_texture;

    InteropTexture2D color_texture;

    InteropTexture2D feature_texture_00_03;
    InteropTexture2D feature_texture_04_07;
    InteropTexture2D feature_texture_08_11;
    InteropTexture2D feature_texture_12_15;

    // geom data
    glm::mat4 inv_view_proj;
	static std::vector<Drawelement> prototype_hq;
	static std::vector<Drawelement> prototype_lq;

    // for frustum culling given in viewspace
    bool hide = false;
    float min_d; 
    float max_d;
};

using SingleViewGeometry = NamedHandle<SingleViewGeometryImpl>;
