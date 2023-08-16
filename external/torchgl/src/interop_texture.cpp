#include "interop_texture.h"
#include "torch_utils.h"
#include <cuda_gl_interop.h>

void CHECK_CUDA(cudaError_t err)
{
  if (err != cudaSuccess)
  {
    std::cerr << "Error: " << cudaGetErrorString(err) << std::endl;
    exit(-1);
  }
}

int copy_texture_to_tensor(const GLenum type, unsigned int tex, torch::Tensor &tensor, size_t height, size_t width, size_t depth=1)
{

  cudaGraphicsResource *res;
  cudaArray_t array;
  CHECK_CUDA(cudaSetDevice(device_index));

  // Register Texture
  CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, type, cudaGraphicsRegisterFlagsReadOnly));

  // Map Texture
  CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
  CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, res, 0, 0));

  // Copy
  for( size_t d = 0; d < depth; ++d ){
    CHECK_CUDA(
      cudaGraphicsSubResourceGetMappedArray( &array, res, d, 0));

    //cout << "Mapped Images" << endl;
    CHECK_CUDA(
      cudaMemcpy2DFromArray( tensor[d].data_ptr(), width, array, 0, 0, width, height, cudaMemcpyDeviceToDevice ));
  }
  // CleanUp
  CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
  CHECK_CUDA(cudaGraphicsUnregisterResource(res));

  return 0;
}

int copy_tensor_to_texture(const GLenum type, unsigned int tex, torch::Tensor &tensor, size_t height, size_t width, size_t depth=1)
{

  cudaGraphicsResource *res;
  cudaArray_t array;

  CHECK_CUDA(cudaSetDevice(device_index));

  // Register Texture
  CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, type, cudaGraphicsRegisterFlagsWriteDiscard));

  // Map Texture
  CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
  CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, res, 0, 0));

  // Copy
  for( size_t d = 0; d < depth; ++d ){
    CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray( &array, res, d, 0 ));

  //cout << "Mapped Images" << endl;
    const void* pointer = tensor[d].data_ptr();
   CHECK_CUDA(cudaMemcpy2DToArray( array, 0, 0, pointer, width, width, height, cudaMemcpyDeviceToDevice ));
  }
  // CleanUp
  CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
  CHECK_CUDA(cudaGraphicsUnregisterResource(res));

  return 0;
}

InteropTexture2DImpl::InteropTexture2DImpl(const std::string& name, const torch::Tensor &tensor)
: name(name)
{    // Check dimensions of tensor
    int tensor_dim = tensor.dim();
    if (tensor_dim != 3)
    {
      std::cerr << "Tensor for tex " << name << "has " << tensor_dim << " dimensions, but expected 3 dimensions [c, w, h]" << std::endl;
      std::cerr << "Aborting copy." << std::endl;
      return;
    }

    // flip height dim
    auto t = tensor.flip({1});
    // move channels to last dim
    t = t.permute({1, 2, 0});

    int channels = t.sizes()[2];
    if (channels > 4){
      std::cerr << "Tensor has " << channels 
          << " channels but we cannot handle more than 4. "
          << "Use multichannels_as_batches() to split the tensor's channel dimension"
          << " and use multiple textures to represent the tensor." << std::endl;

    } else if (channels == 3){
      // Padding of vec3 to vec4 XXX
      t = torch::cat({t, t.new_full({(uint32_t) t.sizes()[0], (uint32_t) t.sizes()[1],1}, 1.0)}, 2);
      channels = 4;
    }

    GLenum type = get_GLenum_from_dtype(t.scalar_type());
    texture2D = cppgl::Texture2D(name, (uint32_t) t.sizes()[1], (uint32_t) t.sizes()[0], cppgl::channels_to_internal_format(type, channels), cppgl::channels_to_format(channels), type, (void*)0);
  
    t = t.contiguous();
    copy_tensor_to_texture(GL_TEXTURE_2D, texture2D->id, t, texture2D->h, texture2D->w * get_dtype_size(t.scalar_type()) * channels);
}

InteropTexture2DImpl::InteropTexture2DImpl(const std::string& name, const cppgl::Texture2D &texture)
: texture2D(texture), name(name)
{}

InteropTexture2DImpl::InteropTexture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
: name(name), texture2D(cppgl::Texture2D(name, w, h, internal_format, format, type, data, mipmap))
{}


torch::Tensor InteropTexture2DImpl::copy_as_tensor() const{
    using namespace torch::indexing;

  // Determine parameters
  int h_tensor = texture2D->h;
  int w_tensor = texture2D->w;

  // Determine channels from format
  int c = texture2D->format == GL_RGBA ? 4 : texture2D->format == GL_RGB ? 3 : texture2D->format == GL_RG ? 2 : 1;
  int c_tensor = c == 3 ? 4 : c; // Padding of vec3 to vec4 XXX
  // if(channels > 0) {
  //   c_tensor = channels;
  // }

  torch::ScalarType d_t = get_dtype_from_GLenum(texture2D->type);

  // if(overwrite_type){
  //   d_t = type;
  //   type_size = type_size;
  // }

  torch::Tensor tensor = torch::empty({h_tensor, w_tensor, c_tensor}, torch::TensorOptions().device(device_str).dtype(d_t));
  copy_texture_to_tensor(GL_TEXTURE_2D, texture2D->id, tensor, (size_t) h_tensor, (size_t) w_tensor*get_dtype_size(d_t) *c_tensor);

  if(c_tensor != c){
    tensor = tensor.index({Slice(), Slice(), Slice(0, c)});
  }

  tensor = tensor.permute({ 2,0,1 });
  tensor = tensor.flip({1});

  return tensor;
}

static bool layout_matches(const cppgl::Texture2D& texture, const torch::Tensor& t){
  bool is_ok = true;
  is_ok &= texture->w == t.sizes()[1];
  is_ok &= texture->h == t.sizes()[0];
  is_ok &= texture->type == get_GLenum_from_dtype(t.scalar_type());
  is_ok &= texture->format == cppgl::channels_to_format(t.sizes()[2]);
  if(!is_ok){
      std::cout << "texture->w == t.sizes()[1]" << (texture->w == t.sizes()[1]) << std::endl;
      std::cout << "texture->h == t.sizes()[0]" << (texture->h == t.sizes()[0]) << std::endl;
      std::cout << "texture->type == get_GLenum_from_dtype(t.scalar_type())" 
        << std::to_string(texture->type == get_GLenum_from_dtype(t.scalar_type())) << std::endl;
      std::cout << "texture->format == cppgl::channels_to_format(t.sizes()[2])" 
        << std::to_string(texture->format == cppgl::channels_to_format(t.sizes()[2])) << std::endl;

  }

  return is_ok; 
}

void InteropTexture2DImpl::copy_from_tensor(const torch::Tensor& tensor){
 // Check dimensions of tensor
    int tensor_dim = tensor.dim();
    if (tensor_dim != 3)
    {
      std::cerr << "Tensor for tex " << name << "has " << tensor_dim << " dimensions, but expected 3 dimensions [c, w, h]" << std::endl;
      std::cerr << "Aborting copy." << std::endl;
      return;
    }

    // flip height dim
    auto t = tensor.flip({1});
    // move channels to last dim
    t = t.permute({1, 2, 0});


    if (!layout_matches(texture2D, t)){
      std::cerr << "Copying of a tensor to the texture " << name << " failed, layout does not match!" << std::endl;  
      return;
    }

    int channels = t.sizes()[2];
    if (channels > 4){
      std::cerr << "Tensor has " << channels 
          << " channels but we cannot handle more than 4. "
          << "Use multichannels_as_batches() to split the tensor's channel dimension"
          << " and use multiple textures to represent the tensor." << std::endl;

    } else if (channels == 3){
      // Padding of vec3 to vec4 XXX
      t = torch::cat({t, t.new_full({(uint32_t) t.sizes()[0], (uint32_t) t.sizes()[1],1}, 1.0)}, 2);
      channels = 4;
    }

    t = t.contiguous();
    copy_tensor_to_texture(GL_TEXTURE_2D, texture2D->id, t, texture2D->h, texture2D->w * get_dtype_size(t.scalar_type()) * channels);
}

InteropTexture2DImpl::~InteropTexture2DImpl(){
    // Texture2D::erase(name);
}

void remove_InteropTexture2D_from_global_maps(const std::string& name){
  cppgl::Texture2D::erase(InteropTexture2D::find(name)->texture2D->name);
  InteropTexture2D::erase(name);

}

InteropTexture2DArrayImpl::InteropTexture2DArrayImpl(const std::string& name, const torch::Tensor &tensor)
: name(name)
{    // Check dimensions of tensor
    int tensor_dim = tensor.dim();
    if (tensor_dim != 3)
    {
      std::cerr << "Tensor for texarray " << name << "has " << tensor_dim << " dimensions, but expected 3 dimensions [c, w, h]" << std::endl;
      std::cerr << "Aborting copy." << std::endl;
      return;
    }

    // flip height dim
    auto t = tensor.flip({1});

    int channels = t.sizes()[0];
    valid_channels = channels;

    // pad to split channels on RGBA texture layers
    if (valid_channels % channels_per_layer != 0) {
      int pad_channels = channels_per_layer- (valid_channels % channels_per_layer);
      t = torch::cat({t, t.new_full({pad_channels, (uint32_t) t.sizes()[1], (uint32_t) t.sizes()[2]}, 1.0)}, 0);
      channels = t.sizes()[0];
    }

    t.unsqueeze_(0);
    t = multichannels_as_batches(t, channels_per_layer);

    // move channels to last dim
    t = t.permute({0, 2, 3, 1});

    GLenum type = get_GLenum_from_dtype(t.scalar_type());
    texture2Darray = cppgl::Texture2DArray(name, (uint32_t) t.sizes()[2], (uint32_t) t.sizes()[1], t.sizes()[0], 
                            cppgl::channels_to_internal_format(type, channels_per_layer), cppgl::channels_to_format(channels_per_layer), type, (void*)0);
  
    t = t.contiguous();
    copy_tensor_to_texture(GL_TEXTURE_2D_ARRAY, texture2Darray->id, t, texture2Darray->h, texture2Darray->w * get_dtype_size(t.scalar_type()) * channels_per_layer, texture2Darray->d);
}

InteropTexture2DArrayImpl::InteropTexture2DArrayImpl(const std::string& name, const cppgl::Texture2DArray &texture)
: texture2Darray(texture), name(name)
{}


InteropTexture2DArrayImpl::InteropTexture2DArrayImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
: name(name), texture2Darray(cppgl::Texture2DArray(name, w, h, d, internal_format, format, type, data, mipmap))
{}



torch::Tensor InteropTexture2DArrayImpl::copy_as_tensor() const{
    using namespace torch::indexing;

  // Determine parameters
  int h_tensor = texture2Darray->h;
  int w_tensor = texture2Darray->w;
  int d_tensor = texture2Darray->d;

  torch::ScalarType d_t = get_dtype_from_GLenum(texture2Darray->type);

  torch::Tensor tensor = torch::empty({d_tensor, h_tensor, w_tensor, channels_per_layer}, torch::TensorOptions().device(device_str).dtype(d_t));
  copy_texture_to_tensor(GL_TEXTURE_2D_ARRAY, texture2Darray->id, tensor, (size_t) h_tensor, (size_t) w_tensor*get_dtype_size(d_t) *channels_per_layer, d_tensor);

  tensor = batches_as_multichannels(tensor);

  if(tensor.sizes()[0] != valid_channels){
    tensor = tensor.index({Slice(0, valid_channels), Slice(), Slice()});
  }

  tensor = tensor.permute({ 2,0,1 });
  tensor = tensor.flip({1});

  return tensor;
  // TODO continue here --> then, go back to decode
}

void remove_InteropTexture2DArray_from_global_maps(const std::string& name){
  cppgl::Texture2DArray::erase(InteropTexture2DArray::find(name)->texture2Darray->name);
  InteropTexture2DArray::erase(name);

}
