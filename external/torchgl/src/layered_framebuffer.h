#pragma once

#include <string>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GL/gl.h>
#include <cppgl.h>

// ------------------------------------------
// LayeredFramebuffer

class LayeredFramebufferImpl {
public:
    LayeredFramebufferImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t D);
    virtual ~LayeredFramebufferImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    LayeredFramebufferImpl(const LayeredFramebufferImpl&) = delete;
    LayeredFramebufferImpl& operator=(const LayeredFramebufferImpl&) = delete;
    LayeredFramebufferImpl& operator=(const LayeredFramebufferImpl&&) = delete; // TODO allow moves?

    inline operator GLuint() const { return id; }

    void bind();
    void unbind();

    void check() const;
    void resize(uint32_t w, uint32_t h);

    void attach_depthbuffer(cppgl::Texture2DArray tex = cppgl::Texture2DArray());
    void attach_colorbuffer(const cppgl::Texture2DArray& tex);

    // data
    const std::string name;
    GLuint id;
    uint32_t w, h, d;
    std::vector<cppgl::Texture2DArray> color_textures;
    std::vector<GLenum> color_targets;
    cppgl::Texture2DArray depth_texture; // TODO continue here? what about depth?
    GLint prev_vp[4];
};

using LayeredFramebuffer = cppgl::NamedHandle<LayeredFramebufferImpl>;
