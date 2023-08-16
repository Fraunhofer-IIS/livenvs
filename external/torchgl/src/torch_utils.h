#pragma once
#ifndef TORCH_UTILS_H
#define TORCH_UTILS_H
#include <cppgl.h>
#include <filesystem>
namespace fs = std::filesystem;
#include <memory>

#include <torch/torch.h> // One-stop header.
#include <torch/script.h>
// #include <torchvision/models/resnet.h>

#include <c10/cuda/CUDAStream.h>
#include <ATen/cuda/CUDAContext.h>


const std::string device_str("cuda:0");
const int device_index = 0;

at::Tensor batches_as_multichannels(const at::Tensor& t);
at::Tensor multichannels_as_batches(const at::Tensor& t, uint32_t channels = 4);
GLenum get_GLenum_from_dtype(const torch::ScalarType d_t);
torch::ScalarType get_dtype_from_GLenum(const GLenum gl_type);
size_t get_dtype_size(const torch::ScalarType dtype);

struct Layout {
    Layout(const std::string& layout_str, bool y_up = false);

    static at::Tensor convert(at::Tensor& t, const Layout& from, const Layout& to);
    at::Tensor to(at::Tensor& t, const Layout& to) const;

    std::string layout_str;
    bool h_up;
};

// XXX TODO test more!
// currently tested TORCH to STBI
const Layout GL_LAYOUT = Layout("hwc", false); 
const Layout STBI_LAYOUT = Layout("hwc", true); 
const Layout TORCH_LAYOUT = Layout("bchw", true); 

//---------------------------------------
// helpers
void print_tensor_info(const at::Tensor& t, bool print_data = false);

void print_model_info(torch::nn::Module model);

// // TODO: template!
// // TODO: check normalization
// // TODO: adapt to make interface consistent with tensorToTexture()

template <typename T> 
at::Tensor textureToTensor_body(cppgl::Texture2D tex, std::vector<T>& pixels, bool normalize = true)
{
    auto type = at::typeMetaToScalarType(caffe2::TypeMeta::Make<T>());
    
    std::cout << "TODO: comment in!!!!!" << std::endl;
    tex->copy_from_gpu(pixels);
    at::Tensor tensor = torch::from_blob(pixels.data(),
        {tex->h, tex->w, cppgl::format_to_channels(tex->format)}, type);

    // In pytorch image tensors are usually represented as channel first.
    tensor = tensor.permute({2, 0, 1});

    if (normalize) {
        //Convert to float
        if constexpr (!std::is_same<T, float>::value) {
            tensor = tensor.toType(at::kFloat);
        }

        // Normalize to [0,1]
        if constexpr (std::is_same<T, uint8_t>::value) {
            tensor = (1.f / 255.f) * tensor;
        }
        tensor = (1.f / 255.f) * tensor;
    }

    return tensor;
}

template <typename T> 
at::Tensor blob_to_tensor(const T* data, uint32_t w, uint32_t h, uint32_t c) {
    auto type = at::typeMetaToScalarType(caffe2::TypeMeta::Make<T>());
    
    at::Tensor tensor = torch::from_blob(data,
        {h, w, c}, type);

    // In pytorch image tensors are usually represented as channel first.
    tensor = tensor.permute({2, 0, 1});

    return tensor;
}

at::Tensor textureToTensor(cppgl::Texture2D tex, bool normalize = true);

torch::jit::script::Module load_module(const fs::path& module_path, const torch::DeviceType device = torch::kCUDA);

void map_value_range(at::Tensor& tensor, const c10::ArrayRef<double>& from, const c10::ArrayRef<double>& to);

void screenshot_tensor_to_path(const torch::Tensor& tensor, const fs::path& path, const Layout& in_layout = TORCH_LAYOUT, const c10::ArrayRef<double>& value_range = c10::ArrayRef<double>({0.,1.}));
void screenshot_tensor_to_path(const torch::Tensor& tensor, const fs::path& path, int i);
void sync_cuda();

// // TODO: template?!
// // TODO: check normalization
// void tensorToTexture(at::Tensor& src_tensor, Texture2D* dest_tex){
//     if (src_tensor.dim() == 4)
//     {
//         src_tensor = src_tensor.squeeze();
//     }

//     // In pytorch image tensors are usually represented as channel first.
//     src_tensor = src_tensor.permute({1, 2, 0});
//     src_tensor = src_tensor.cpu();

//     int w = src_tensor.size(0);
//     int h = src_tensor.size(1);
//     int c = src_tensor.size(2);

//     // Normalize to [0,1]
//     // if constexpr (std::is_same<ScalarType, unsigned char>::value)
//     // {
//         src_tensor = .5f * (src_tensor+ torch::ones({w,h,c}));
//         src_tensor = src_tensor.clamp(0.f, 1.f);
//     //     tensor = tensor.toType(at::kByte);
//     // }
//     std::vector<float> pixels(w*h*c);    
//     pixels.assign(src_tensor.data_ptr<float>(), src_tensor.data_ptr<float>()+(w*h*c));
//     // map to gpu
//     std::cout << "TODO: comment in!!!!!" << std::endl;
//     // dest_tex->copy_from_cpu(pixels);
// }
#endif