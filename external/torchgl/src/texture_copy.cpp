// #include "texture_copy.h"
// #include "torch_utils.h"
// #include <cuda_gl_interop.h>
// #include <torch/torch.h>

// using namespace std;

// void CHECK_CUDA(cudaError_t err) {
//     if(err != cudaSuccess) {
//         std::cerr << "Error: " << cudaGetErrorString(err) << std::endl;
//         exit(-1);
//     }
// }

// int texturearray_to_tensor( unsigned int tex, torch::Tensor& tensor, const size_t height, const size_t width, const size_t depth ){
//   //cout << "Copying tensor into texture ..." << endl;

//   cudaGraphicsResource* res; cudaArray_t array;

//   CHECK_CUDA(cudaSetDevice( 0 ));

//   //cout << "Device set" << endl;

//   // Register Texture
//   CHECK_CUDA(
//     cudaGraphicsGLRegisterImage( &res, tex, GL_TEXTURE_2D_ARRAY, cudaGraphicsRegisterFlagsReadOnly ));
//   //cout << "Registered Images" << endl;
//     // Map Texture
//   CHECK_CUDA(cudaGraphicsMapResources( 1, &res, 0));

//   for( size_t d = 0; d < depth; ++d ){
//     CHECK_CUDA(
//       cudaGraphicsSubResourceGetMappedArray( &array, res, d, 0));

//     //cout << "Mapped Images" << endl;
//     CHECK_CUDA(
//       cudaMemcpy2DFromArray( tensor[d].data_ptr(), width, array, 0, 0, width, height, cudaMemcpyDeviceToDevice ));
//   }
  
//   // CleanUp
//   CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsUnregisterResource(res));

//   return 0;
// }


// int tensor_to_texturearray( unsigned int tex, torch::Tensor& tensor, const size_t height, const size_t width, const size_t depth ){
//   //cout << "Copying tensor into texture ..." << endl;

//   cudaGraphicsResource* res; cudaArray_t array;

//   CHECK_CUDA(cudaSetDevice( 0 ));

//   //cout << "Device set" << endl;

//   // Register Texture
//   CHECK_CUDA(cudaGraphicsGLRegisterImage( &res, tex, GL_TEXTURE_2D_ARRAY, cudaGraphicsRegisterFlagsWriteDiscard ));

//   //cout << "Registered Images" << endl;

//   // Map Texture
//   CHECK_CUDA(cudaGraphicsMapResources( 1, &res, 0 ));

//   for( size_t d = 0; d < depth; ++d ){
//     CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray( &array, res, d, 0 ));

//   //cout << "Mapped Images" << endl;
//     const void* pointer = tensor[d].data_ptr();
//    CHECK_CUDA(cudaMemcpy2DToArray( array, 0, 0, pointer, width, width, height, cudaMemcpyDeviceToDevice ));
//   }
//   // CleanUp
//   CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsUnregisterResource(res));

//   return 0;
// }

// torch::Tensor texture2Darray_to_tensor(Texture2DArray& tex){
//   using namespace torch::indexing;

//   // Determine parameters
//   int h_tensor = tex->h;
//   int w_tensor = tex->w;
//   int d_tensor = tex->d;

//   // Determine channels from format
//   // TODO: Test (512*3%4 == 0, should work, but does not). Use opengl funcs to read alignment.
//   GLint data;

//   int c = tex->format == GL_RGBA ? 4 : tex->format == GL_RGB ? 3 : tex->format == GL_RG ? 2 : 1;
//   int c_tensor = c == 3 ? 4 : c; // Padding of vec3 to vec4 XXX
//   // if(channels > 0) {
//   //   c_tensor = channels;
//   // }

//   torch::ScalarType d_t;
//   int type_size;
//   get_dtype_from_GLenum(tex->type, d_t, type_size);

//   // if(overwrite_type){
//   //   d_t = type;
//   //   type_size = type_size;
//   // }

//   // const std::string device_str("cuda:0");
//   torch::Tensor tensor = torch::empty({d_tensor, h_tensor, w_tensor, c_tensor}, torch::TensorOptions().device(device_str).dtype(d_t));
//   texturearray_to_tensor(tex->id, tensor, (size_t) min(tex->h, h_tensor), (size_t) min(tex->w, w_tensor)*type_size*c_tensor, d_tensor);

//   if(c_tensor != c){
//     tensor = tensor.index({Slice(), Slice(), Slice(), Slice(0, c)});
//   }

//   tensor = tensor.permute({ 0,3,1,2 });
//   tensor = tensor.flip({2});

//   return tensor;
// }

// void tensor_to_texture2Darray(torch::Tensor& t, Texture2DArray& tex, bool ignore_type_check){
//   // Check dimensions of tensor
//   int tensor_dim = t.dim();
//   if(tensor_dim != 4){
//     cerr << "Tensor has " << tensor_dim << " dimensions, but expected 4 dimensions [bc, c, w, h]" << endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   auto tensor = t.flip({2});
//   tensor = tensor.permute({ 0,2,3,1 });

//   int height_tensor = tensor.size(1);
//   int width_tensor = tensor.size(2);
//   int d_tensor = tensor.size(0);
//   int c_tensor = tensor.size(3);

//   // Check for dimension mismatch
//   if (tex->w != width_tensor || tex->h != height_tensor || tex->d != d_tensor) {
//     cerr << "Dimension mismatch!" << endl;
//     cerr  << tex->w << " != " << width_tensor << " or " 
//           << tex->h << " != " << height_tensor << " or " 
//           << tex->d << " != " << d_tensor
//           << std::endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   // Check channels
//   // XXX: @ linus: how did you solve the padding here?
//   int c = tex->format == GL_RGBA ? 4 : tex->format == GL_RGB ? 3 : tex->format == GL_RG ? 2 : 1;

//   if( c_tensor == 3 ){
//     const at::Tensor zeros = tensor.new_zeros({tensor.size(0), tensor.size(1), tensor.size(2), 1});
//     tensor = torch::cat({tensor, zeros}, 3); 
//     c_tensor = 4;
//   }
//   if(c != c_tensor){
//     cerr << "Number of channels in tensor (" << c_tensor << ") is not equal to the number of channels in texture(" << c << ")." << endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   // Check types
//   torch::ScalarType texture_type;
//   int type_size;
//   get_dtype_from_GLenum(tex->type, texture_type, type_size);
//   torch::ScalarType tensor_type = tensor.scalar_type();

//   if(!ignore_type_check && texture_type != tensor_type){
//     cerr << "Types of tensor and texture do not match." << endl;
//     cerr << "Aborting copy." << endl;
//   }

//   tensor = tensor.contiguous();
//   tensor_to_texturearray(tex->id, tensor, (size_t) height_tensor, (size_t) width_tensor*type_size*c_tensor, d_tensor);
// }

// int texture_to_tensor(unsigned int tex, torch::Tensor& tensor, size_t height, size_t width){
//   // cout << "Copying tensor into texture ..." << endl;

//   cudaGraphicsResource* res; cudaArray_t array;

//   CHECK_CUDA(cudaSetDevice(0));

//   // cout << "Device set" << endl;

//   // Register Texture
//   CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly));

//   // cout << "Registered Images" << endl;

//   // Map Texture
//   CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray( &array, res, 0, 0));

//   // cout << "Mapped Images" << endl;

//   CHECK_CUDA(cudaMemcpy2DFromArray(tensor.data_ptr(), width, array, 0, 0, width, height, cudaMemcpyDeviceToDevice));
  
//   // cout << "clean up" << endl;
//   // CleanUp
//   CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsUnregisterResource(res));

//   return 0;
// }

// int tensor_to_texture(unsigned int tex, torch::Tensor& tensor,  size_t height, size_t width){
//   //cout << "Copying tensor into texture ..." << endl;

//   cudaGraphicsResource* res; cudaArray_t array;

//   CHECK_CUDA(cudaSetDevice(0));

//   //cout << "Device set" << endl;

//   // Register Texture
//   CHECK_CUDA(cudaGraphicsGLRegisterImage(&res, tex, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard));

//   //cout << "Registered Images" << endl;

//   // Map Texture
//   CHECK_CUDA(cudaGraphicsMapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray( &array, res, 0, 0));

//   //cout << "Mapped Images" << endl;

//   CHECK_CUDA(cudaMemcpy2DToArray(array, 0, 0, tensor.data_ptr(), width, width, height, cudaMemcpyDeviceToDevice));

//   // CleanUp
//   CHECK_CUDA(cudaGraphicsUnmapResources(1, &res, 0));
//   CHECK_CUDA(cudaGraphicsUnregisterResource(res));

//   return 0;
// }

// torch::Tensor texture2D_to_tensor(Texture2D& tex){
//   using namespace torch::indexing;

//   // Determine parameters
//   int h_tensor = tex->h;
//   int w_tensor = tex->w;

//   // Determine channels from format
//   // TODO: Test (512*3%4 == 0, should work, but does not). Use opengl funcs to read alignment.
//   GLint data;

//   int c = tex->format == GL_RGBA ? 4 : tex->format == GL_RGB ? 3 : tex->format == GL_RG ? 2 : 1;
//   int c_tensor = c == 3 ? 4 : c; // Padding of vec3 to vec4 XXX
//   // if(channels > 0) {
//   //   c_tensor = channels;
//   // }

//   torch::ScalarType d_t;
//   int type_size;
//   get_dtype_from_GLenum(tex->type, d_t, type_size);

//   // if(overwrite_type){
//   //   d_t = type;
//   //   type_size = type_size;
//   // }

//   torch::Tensor tensor = torch::empty({h_tensor, w_tensor, c_tensor}, torch::TensorOptions().device(device_str).dtype(d_t));
//   texture_to_tensor(tex->id, tensor, (size_t) min(tex->h, h_tensor), (size_t) min(tex->w, w_tensor)*type_size*c_tensor);

//   if(c_tensor != c){
//     tensor = tensor.index({Slice(), Slice(), Slice(0, c)});
//   }

//   tensor = tensor.permute({ 2,0,1 });
//   tensor = tensor.flip({1});

//   return tensor;
// }

// void tensor_to_texture2D(torch::Tensor& t, Texture2D& tex, bool ignore_type_check){
//   // Check dimensions of tensor
//   int tensor_dim = t.dim();
//   if(tensor_dim != 3){
//     cerr << "Tensor has 4 dimensions, but expected 3 dimensions [w, h, c]" << endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   auto tensor = t.flip({1});
//   tensor = tensor.permute({ 1,2,0 });

//   int height_tensor = tensor.size(0);
//   int width_tensor = tensor.size(1);
//   int c_tensor = tensor.size(2);

//   // Check for dimension mismatch
//   if (tex->w != width_tensor || tex->h != height_tensor) {
//     cerr << "Dimension mismatch!" << endl;
//     cerr << tex->w << " != " << width_tensor << " or " << tex->h << " != " << height_tensor << std::endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   // Check channels
//   // XXX: @ linus: how did you solve the padding here?
//   int c = tex->format == GL_RGBA ? 4 : tex->format == GL_RGB ? 4 : tex->format == GL_RG ? 2 : 1;
//   if( c_tensor == 3 ){
//     const at::Tensor zeros = tensor.new_zeros({tensor.size(0), tensor.size(1), 1});
//     tensor = torch::cat({tensor, zeros}, 2);
//     c_tensor = 4;
//   }
//   if(c != c_tensor){
//     cerr << "Number of channels in tensor (" << c_tensor << ") is not equal to the number of channels in texture(" << c << ")." << endl;
//     cerr << "Aborting copy." << endl;
//     return;
//   }

//   // Check types
//   torch::ScalarType texture_type;
//   int type_size;
//   get_dtype_from_GLenum(tex->type, texture_type, type_size);
//   torch::ScalarType tensor_type = tensor.scalar_type();

//   if(!ignore_type_check && texture_type != tensor_type){
//     cerr << "Types of tensor and texture do not match." << endl;
//     cerr << "Aborting copy." << endl;
//   }
//   tensor = tensor.contiguous();
//   tensor_to_texture(tex->id, tensor, (size_t) height_tensor, (size_t) width_tensor*type_size*c_tensor);
// }
