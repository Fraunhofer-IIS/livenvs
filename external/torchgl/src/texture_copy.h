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
int texture_to_tensor(unsigned int tex,  torch::Tensor& tensor,  size_t height, size_t width);
int tensor_to_texture(unsigned int tex, torch::Tensor& tensor, size_t height, size_t width);


// Functions for copying between a Texture2D and tensor. These functions take only the tensor/texture and try to infer as much information as possible.
// NOTE: Prototype, Not tested throughly

/* Copying from a Texture2D to a tensor. 
*  Functions with more parameters can be used to manually set attributes such as the number of channels, ...
*  Otherwise, these functions attempt to find out the correct values by themselves.
*  tex             :- Texture2D to copy from
*  height          :- Height of the PyTorch Tensor/OpenGL texture
*  width           :- Width of the PyTorch Tensor/OpenGL texture (Note: NOT in Bytes)
*  channels        :- Number of channels
*  overwrite_type  :- Should the inferred type be overriden by the given one (Used in the implementation of the second function)
*  type            :- Type to be used for the PyTorch Tensor
*  type_size       :- Size of the PyTorch Tensor type 
*  return          :- A PyTorch Tensor containing the data from the Texture2D
*/

torch::Tensor texture2D_to_tensor(Texture2D& tex);

/* Copying from a Texture2D to a tensor. 
*  Functions with more parameters can be used to manually set attributes such as the number of channels, ...
*  Otherwise, these functions attempt to find out the correct values by themselves.
*  tensor             :- Tensor to copy from
*  tex                :- Texture2D to copy into
*  height             :- Height of the PyTorch Tensor/OpenGL texture
*  width              :- Width of the PyTorch Tensor/OpenGL texture (Note: NOT in Bytes)
*  ignore_type_check  :- Ignore the check making sure that the tensor and texture types match
*/
void tensor_to_texture2D(torch::Tensor& tensor, cppgl::Texture2D& tex, bool ignore_type_check);


torch::Tensor texture2Darray_to_tensor(cppgl::Texture2DArray& tex);
void tensor_to_texture2Darray(torch::Tensor& t, cppgl::Texture2DArray& tex, bool ignore_type_check);
