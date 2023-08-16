#include "layered_framebuffer.h"
#include <atomic>

LayeredFramebufferImpl::LayeredFramebufferImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d) 
        : name(name), id(0), w(w), h(h), d(d) {
    glGenFramebuffers(1, &id);
}

LayeredFramebufferImpl::~LayeredFramebufferImpl() {
    glDeleteFramebuffers(1, &id);
}

void LayeredFramebufferImpl::bind() {
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glDrawBuffers(color_targets.size(), color_targets.data());
}

void LayeredFramebufferImpl::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
}

void LayeredFramebufferImpl::check() const {
    if (!(depth_texture && *depth_texture))
        throw std::runtime_error("ERROR: Framebuffer: depth buffer not present or invalid!");
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::string s;
        if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            s = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            s = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
            s = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
            s = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        throw std::runtime_error("ERROR: Framebuffer incomplete! Status: " + s);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LayeredFramebufferImpl::resize(uint32_t w, uint32_t h) {
    this->w = w;
    this->h = h;
    this->d = d;
    if (depth_texture)
        depth_texture->resize(w, h, d);
    for (auto& tex : color_textures)
        tex->resize(w, h, d);
}

void LayeredFramebufferImpl::attach_depthbuffer(cppgl::Texture2DArray tex) {
    std::cout << w << ", " << h << ", "  << d << std::endl;
    if (!tex) tex = cppgl::Texture2DArray(name + "_default_depthbuffer", w, h, d, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    depth_texture = tex;
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex->id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LayeredFramebufferImpl::attach_colorbuffer(const cppgl::Texture2DArray& tex) {
    const GLenum target = GL_COLOR_ATTACHMENT0 + color_targets.size();
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferTexture(GL_FRAMEBUFFER, target, tex->id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    color_textures.push_back(tex);
    color_targets.push_back(target);
}
