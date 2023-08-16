#pragma once

#include <memory>
#include <filesystem>
namespace fs = std::filesystem;
#include <GL/glew.h>
#include <GL/gl.h>
#include "named_handle.h"
#include <vector>
#include <math.h>

CPPGL_NAMESPACE_BEGIN

// ----------------------------------------------------
// helper funcs

static inline uint32_t format_to_channels(GLint format) {
    return format == GL_RGBA ? 4 : format == GL_RGB ? 3 : format == GL_RG ? 2 : 1;
}
static inline GLint channels_to_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : channels == 2 ? GL_RG : GL_RED;
}
static inline GLint channels_to_float_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
}
static inline GLint channels_to_ubyte_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
}
static inline GLint channels_to_internal_format(GLenum type, uint32_t channels) {
    switch (type){
        case GL_FLOAT:
            return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
        case GL_HALF_FLOAT:
            return channels == 4 ? GL_RGBA16F : channels == 3 ? GL_RGB16F : channels == 2 ? GL_RG16F : GL_R16F;
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
        case GL_SHORT:
            return channels == 4 ? GL_RGBA16 : channels == 3 ? GL_RGB16 : channels == 2 ? GL_RG16 : GL_R16;
        case GL_INT:
            return channels == 4 ? GL_RGBA32I : channels == 3 ? GL_RGB32I : channels == 2 ? GL_RG32I : GL_R32I;
        default:
            std::cerr << "Unknown type + channel" << std::endl;
            return GL_RGBA32F;
    }
}

// ----------------------------------------------------
// Texture2D

class Texture2DImpl {
public:
    // construct from image on disk
    Texture2DImpl(const std::string& name, const fs::path& path, bool mipmap = true);
    // construct empty texture or from raw data
    Texture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type,
            const void* data = 0, bool mipmap = false);
    virtual ~Texture2DImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    // // Texture2DImpl(const Texture2DImpl&) = delete;
    // Texture2DImpl& operator=(const Texture2DImpl&) = delete;
    // Texture2DImpl& operator=(const Texture2DImpl&&) = delete;

    explicit inline operator bool() const  { return w > 0 && h > 0 && glIsTexture(id); }
    inline operator GLuint() const { return id; }

    // resize (discards all data!)
    void resize(uint32_t w, uint32_t h);

    // bind/unbind to/from OpenGL
    virtual void bind(uint32_t uint) const;
    void unbind() const;
    void bind_image(uint32_t unit, GLenum access, GLenum format, uint32_t level = 0u) const;
    void unbind_image(uint32_t unit) const;

    // CPU <-> GPU data transfers
    template <typename T>
    void copy_from_gpu(std::vector<T>& destination, bool flip = false) const{
        static_assert(std::is_same<T, float>::value || std::is_same<T, uint8_t>::value, "bad type bro: needs to be either float or uint8_t!");

        destination.clear();
        destination.resize(w * h * format_to_channels(format));
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexImage(GL_TEXTURE_2D, 0, format, 
            std::is_same<T, float>::value ? GL_FLOAT : GL_UNSIGNED_BYTE,
            &destination[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    template <typename T>
    void copy_from_gpu(std::vector<T>& destination, int level, bool flip = false) const{
        static_assert(std::is_same<T, float>::value || std::is_same<T, uint8_t>::value, "bad type bro: needs to be either float or uint8_t!");

        destination.clear();
        destination.resize((w/std::pow(2.0, level)) * (h/std::pow(2.0, level)) * format_to_channels(format));
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexImage(GL_TEXTURE_2D, level, format, 
            std::is_same<T, float>::value ? GL_FLOAT : GL_UNSIGNED_BYTE,
            &destination[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    template <typename T>
    void copy_to_gpu(const std::vector<T>& source, bool mipmap= false) {
        static_assert((std::is_same<T, float>::value && type == GL_FLOAT) || (std::is_same<T, uint8_t>::value && type == GL_UNSIGNED_BYTE), "bad type bro: needs to be either float or uint8_t!");

        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, source.data());
        if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // save to disk
    void save_ldr(const fs::path& path, bool flip = true, bool async = false) const;

    // data
    const std::string name;
    const fs::path loaded_from_path;
    GLuint id;
    int w, h;
    GLint internal_format;
    GLenum format, type;

    size_t calc_tex_size();
    void add_to_storage_counter();
    void substract_from_storage_counter();
    static size_t accum_texture_storage;

};

using Texture2D = NamedHandle<Texture2DImpl>;
template class _API NamedHandle<Texture2DImpl>; //needed for Windows DLL export

// ----------------------------------------------------
// Texture3D

class Texture3DImpl {
public:
    // construct empty texture or from raw data
    Texture3DImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type,
            const void *data = 0, bool mipmap = false);
    virtual ~Texture3DImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    Texture3DImpl(const Texture3DImpl&) = delete;
    Texture3DImpl& operator=(const Texture3DImpl&) = delete;
    Texture3DImpl& operator=(const Texture3DImpl&&) = delete;

    explicit inline operator bool() const  { return w > 0 && h > 0 && d > 0 && glIsTexture(id); }
    inline operator GLuint() const { return id; }

    // resize (discards all data!)
    void resize(uint32_t w, uint32_t h, uint32_t d);

    // bind/unbind to/from OpenGL
    void bind(uint32_t uint) const;
    void unbind() const;
    void bind_image(uint32_t unit, GLenum access, GLenum format) const;
    void unbind_image(uint32_t unit) const;

    // TODO CPU <-> GPU data transfers

    // data
    const std::string name;
    GLuint id;
    int w, h, d;
    GLint internal_format;
    GLenum format, type;
};

using Texture3D = NamedHandle<Texture3DImpl>;
template class _API NamedHandle<Texture3DImpl>; //needed for Windows DLL export


// ----------------------------------------------------
// Texture2DArray

class Texture2DArrayImpl {
public:
    // construct from image on disk
    Texture2DArrayImpl(const std::string& name, const std::vector<fs::path>& path, bool mipmap = true);
    // construct empty texture or from raw data
    Texture2DArrayImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type,
            const void *data = 0, bool mipmap = false);
    virtual ~Texture2DArrayImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    Texture2DArrayImpl(const Texture2DArrayImpl&) = delete;
    Texture2DArrayImpl& operator=(const Texture2DArrayImpl&) = delete;
    Texture2DArrayImpl& operator=(const Texture2DArrayImpl&&) = delete;

    explicit inline operator bool() const  { return w > 0 && h > 0 && glIsTexture(id); }
    inline operator GLuint() const { return id; }

    // resize (discards all data!)
    void resize(uint32_t w, uint32_t h, uint32_t d);

    // bind/unbind to/from OpenGL
    void bind(uint32_t uint) const;
    void unbind() const;
    void bind_image(uint32_t unit, uint32_t layer, GLenum access, GLenum format, uint32_t level = 0u) const;
    void unbind_image(uint32_t unit) const;

    // save to disk
    void save_png(const fs::path& path, bool flip = true) const;
    void save_jpg(const fs::path& path, int quality = 100, bool flip = true) const; // quality: [1, 100]

    // data
    const std::string name;
    GLuint id;
    int w, h, d;
    GLint internal_format;
    GLenum format, type;
};

using Texture2DArray = NamedHandle<Texture2DArrayImpl>;
template class _API NamedHandle<Texture2DArrayImpl>; //needed for Windows DLL export

CPPGL_NAMESPACE_END
