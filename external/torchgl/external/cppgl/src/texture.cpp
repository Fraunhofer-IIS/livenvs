#include "texture.h"
#include <vector>
#include <iostream>
#include "image_load_store.h"

CPPGL_NAMESPACE_BEGIN

// ----------------------------------------------------
// Texture2D

size_t Texture2DImpl::accum_texture_storage = 0;

size_t get_typesize_from_GLenum(const GLenum type){
  // GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_HALF_FLOAT, GL_FLOAT

  size_t type_size = 0;
  switch (type) {
    case GL_UNSIGNED_BYTE:
      type_size = 1;
      break;
    case GL_BYTE:
      type_size = 1;
      break;
    case GL_UNSIGNED_SHORT:
      type_size = 2;
      break;
    case GL_SHORT:
      type_size = 2;
      break;
    case GL_UNSIGNED_INT:
      type_size = 4;
      break;
    case GL_INT:
      type_size = 4;
      break;
    case GL_HALF_FLOAT:
      type_size = 2;
      break;
    case GL_FLOAT:
      type_size = 4;
      break;

    default:
      std::cerr << "Warning: Texture uses unknown type with id " << type << std::endl;
  }

    return type_size;
}

size_t Texture2DImpl::calc_tex_size(){
     size_t channels = format_to_channels(format);
    channels = channels == 3? 4 : channels;
    size_t typesize = get_typesize_from_GLenum(type);

    size_t  bytes = w*h*channels*typesize;
    return bytes;
}

void Texture2DImpl::add_to_storage_counter(){
    accum_texture_storage += calc_tex_size();
}

void Texture2DImpl::substract_from_storage_counter(){
    accum_texture_storage -= calc_tex_size();
}

Texture2DImpl::Texture2DImpl(const std::string& name, const fs::path& path, bool mipmap) : name(name), loaded_from_path(path), id(0) {
    // load image from disk
    auto [data, w_out, h_out, channels, is_hdr] = image_load(path);
    std::cout << "loaded" << path << std::endl;
    this->w = w_out;
    this->h = h_out;

    if (is_hdr) {
        internal_format = channels_to_float_format(channels);
        type = GL_FLOAT;
    // } else if (stbi_is_16_bit(path.string().c_str())) {
    //     std::cout << path << std::endl;
    //     data = (uint8_t*)stbi_load_16(path.string().c_str(), &w, &h, &channels, 0);
    //     uint16_t* data_16 = (uint16_t*)data;
    //     internal_format = channels_to_float_format(channels);
    //     format = channels_to_format(channels);
    //     type = GL_UNSIGNED_SHORT;
    //     uint16_t min = UINT16_MAX, max = 0;
    //     for(int i =0; i < w*h*channels; ++i) {
    //         min = std::min(min, data_16[i]);
    //         max = std::max(max, data_16[i]);
    //         // if (data_16[i]!=0)
    //         //     std::cout << data_16[i] << ", ";
    //         }
    //     std::cout << std::endl;
    //     std::cout << min << ", " << max << std::endl;
    } else {
        internal_format = channels_to_ubyte_format(channels);
        type = GL_UNSIGNED_BYTE;
    }
    format = channels_to_format(channels);

    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    //opengl by default needs 4 byte alignment after every row
    //stbi loaded data is not aligned that way -> pixelStore attributes need to be set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, &data[0]);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

}

Texture2DImpl::Texture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
    : name(name), id(0), w(w), h(h), internal_format(internal_format), format(format), type(type) {
    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL) ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            mipmap ? GL_LINEAR_MIPMAP_LINEAR : (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL) ? GL_NEAREST : GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, data);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    add_to_storage_counter();
}

Texture2DImpl::~Texture2DImpl() {
    if (glIsTexture(id))
        glDeleteTextures(1, &id);

    substract_from_storage_counter();
}

void Texture2DImpl::resize(uint32_t w, uint32_t h) {
    substract_from_storage_counter();

    this->w = w;
    this->h = h;
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    add_to_storage_counter();
}

void Texture2DImpl::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2DImpl::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2DImpl::bind_image(uint32_t unit, GLenum access, GLenum format, uint32_t level) const {
    glBindImageTexture(unit, id, level, GL_FALSE, 0, access, format);
}

void Texture2DImpl::unbind_image(uint32_t unit) const {
    glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}

void Texture2DImpl::save_ldr(const fs::path& path, bool flip, bool async) const {
    std::vector<uint8_t> pixels(size_t(w) * h * format_to_channels(format));
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    image_store_ldr(path, pixels.data(), w, h, format_to_channels(format), flip, async);
}



// ----------------------------------------------------
// Texture3D

Texture3DImpl::Texture3DImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
    : name(name), id(0), w(w), h(h), d(d), internal_format(internal_format), format(format), type(type) {
    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_3D, id);
    // default border color is (0, 0, 0, 0)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, w, h, d, 0, format, type, data);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
}

Texture3DImpl::~Texture3DImpl() {
    if (glIsTexture(id))
        glDeleteTextures(1, &id);
}

void Texture3DImpl::resize(uint32_t w, uint32_t h, uint32_t d) {
    this->w = w;
    this->h = h;
    this->d = d;
    glBindTexture(GL_TEXTURE_3D, id);
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, w, h, d, 0, format, type, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3DImpl::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, id);
}

void Texture3DImpl::unbind() const {
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3DImpl::bind_image(uint32_t unit, GLenum access, GLenum format) const {
    glBindImageTexture(unit, id, 0, GL_FALSE, 0, access, format);
}

void Texture3DImpl::unbind_image(uint32_t unit) const {
    glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}

// ----------------------------------------------------
// Texture2DArray

Texture2DArrayImpl::Texture2DArrayImpl(const std::string& name, const std::vector<fs::path>& paths, bool mipmap) : name(name), id(0) {

    d = paths.size();
    auto [first_subimg_data, w_out, h_out, channels, is_hdr] = image_load(paths[0]);

    this->w = w_out;
    this->h = h_out;

    if (is_hdr) {
        internal_format = channels_to_float_format(channels);
        type = GL_FLOAT;
    } else {
        internal_format = channels_to_ubyte_format(channels);
        type = GL_UNSIGNED_BYTE;
    }
    format = channels_to_format(channels);
   
    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    // default border color is (0, 0, 0, 0)
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, w, h, d, 0, format, type, (void*)0);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, w, h, 1, format, type, &first_subimg_data[0]);

    // load all remaining images and copy to device
    // TODO: check for mismatching channels, format or type
    for(int i = 1; i < d; ++i){ 
        auto [data, w_out, h_out, channels, is_hdr] = image_load(paths[0]);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, format, type, &data[0]);
    }


    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    // free data
}

Texture2DArrayImpl::Texture2DArrayImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
    : name(name), id(0), w(w), h(h), d(d), internal_format(internal_format), format(format), type(type) {
    // init GL texture
     // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    // default border color is (0, 0, 0, 0)
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, format == GL_DEPTH_COMPONENT ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
            mipmap ? GL_LINEAR_MIPMAP_LINEAR : format == GL_DEPTH_COMPONENT ? GL_NEAREST : GL_LINEAR);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, w, h, d, 0, format, type, data);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

Texture2DArrayImpl::~Texture2DArrayImpl() {
    if (glIsTexture(id)){
        glDeleteTextures(1, &id);
    }
}

void Texture2DArrayImpl::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
}

void Texture2DArrayImpl::unbind() const {
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture2DArrayImpl::resize(uint32_t w, uint32_t h, uint32_t d) {
    this->w = w;
    this->h = h;
    this->d = d;
    glBindTexture(GL_TEXTURE_2D_ARRAY, id);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, w, h, d, 0, format, type, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void Texture2DArrayImpl::bind_image(uint32_t unit, uint32_t layer, GLenum access, GLenum format, uint32_t level) const{
    glBindImageTexture(unit, id, level, GL_TRUE, layer, access, format);
}

void Texture2DArrayImpl::unbind_image(uint32_t unit) const {
    glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}

// Texture2DArrayImpl::create_view(uint32_t unit) const {
//     glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
// // }

//  view_id;
//                         glGenTextures(1, &view_id);
//                         glBindTexture(GL_TEXTURE_2D_ARRAY, view_id);
//                         glTextureView(view_id, GL_TEXTURE_2D_ARRAY, holefilling_fused_feat->id, 
//                             holefilling_fused_feat->internal_format, 
//                             l+1, 1, 
//                             0, holefilling_fused_feat->d);

//                         glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
//                         mipmaplevel_views.push_back(view_id);
// glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
CPPGL_NAMESPACE_END
