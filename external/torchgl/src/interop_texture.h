#pragma once

#include <string>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GL/gl.h>
#include "stdio.h"
#include <cppgl.h>
#include <iostream>
#include <utility>
#include <torch/torch.h>



/* Standard functions for copying between textures and tensors
*  tex    :- id of the OpenGL texture
*  tensor :- PyTorch Tensor
*  height :- Height of the PyTorch Tensor/OpenGL texture
*  width  :- Width of the PyTorch Tensor/OpenGL texture in BYTES (width*size_of_type*number_of_channels)
*  return :- Always 0 (meant as an error code, however, function exits on error ...) 
*/
int copy_texture_to_tensor(unsigned int tex,  torch::Tensor& tensor,  size_t height, size_t width);
int copy_tensor_to_texture(unsigned int tex, torch::Tensor& tensor, size_t height, size_t width);


// ------------------------------------------
// InteropTexture2D

class InteropTexture2DImpl {
public:
    InteropTexture2DImpl(const std::string& name);
    InteropTexture2DImpl(const std::string& name, const torch::Tensor &tensor);
    InteropTexture2DImpl(const std::string& name, const cppgl::Texture2D &texture);
    InteropTexture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type, 
        const void* data = 0, bool mipmap = false);
    virtual ~InteropTexture2DImpl();
    
    // inline operator Texture2DImpl() const { return static_cast<Texture2DImpl>(*this); }

    torch::Tensor copy_as_tensor() const;
    void copy_from_tensor(const torch::Tensor& tensor);
    // Texture2D& get_tex(){ return texture2D; }
    // inline operator Texture2D&(){ return texture2D; }
    // inline operator Texture2D(){ return texture2D; }

    // data
    std::string name;
    cppgl::Texture2D texture2D;
};

using InteropTexture2D = cppgl::NamedHandle<InteropTexture2DImpl>;
void remove_InteropTexture2D_from_global_maps(const std::string& name);


// ------------------------------------------
// InteropTexture2DArray

class InteropTexture2DArrayImpl {
public:
    InteropTexture2DArrayImpl(const std::string& name);
    InteropTexture2DArrayImpl(const std::string& name, const torch::Tensor& tensor);
    InteropTexture2DArrayImpl(const std::string& name, const cppgl::Texture2DArray &texture2Darray);
    InteropTexture2DArrayImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type, 
        const void* data = 0, bool mipmap = false);


    torch::Tensor copy_as_tensor() const;
    void copy_from_tensor(const torch::Tensor& tensor);

    // data
    std::string name;
    int valid_channels;
    const int channels_per_layer = 4;
    cppgl::Texture2DArray texture2Darray;
};

using InteropTexture2DArray = cppgl::NamedHandle<InteropTexture2DArrayImpl>;
void remove_InteropTexture2DArray_from_global_maps(const std::string& name);