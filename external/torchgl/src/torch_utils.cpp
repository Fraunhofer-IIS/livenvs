#include "torch_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/cppgl/src/stbi/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/cppgl/src/stbi/stb_image_write.h"

at::Tensor batches_as_multichannels(const at::Tensor &t)
{
    auto in_sizes = t.sizes();
    auto tensor = t.view({-1, in_sizes[2], in_sizes[3]});
    return tensor;
}

at::Tensor multichannels_as_batches(const at::Tensor &t, uint32_t channels)
{
    auto in_sizes = t.sizes();
    if (in_sizes[in_sizes.size() - 3] % channels != 0)
    {
        std::cerr << "number of channels of tensor is not divisible by number of channels per texture" << std::endl;
    }
    auto tensor = t.view({in_sizes[in_sizes.size() - 3] / channels, channels, in_sizes[in_sizes.size() - 2], in_sizes[in_sizes.size() - 1]});
    return tensor;
}

size_t get_dtype_size(const torch::ScalarType dtype) {
    return torch::elementSize(dtype);
}

GLenum get_GLenum_from_dtype(const torch::ScalarType d_t) {
    switch (d_t) {
        case torch::kUInt8:
            return GL_UNSIGNED_BYTE;
        case torch::kInt8:
            return GL_BYTE;
        case torch::kInt16:
            return GL_SHORT;
        case torch::kInt32:
            return GL_INT;
        case torch::kFloat16:
            return GL_HALF_FLOAT;
        case torch::kFloat32:
            return GL_FLOAT;

        default:
            // TODO: Print name?
            std::cerr << "Warning: Tensor is of unknown type " << d_t << std::endl;
            return GL_FLOAT;
    }
}

torch::ScalarType get_dtype_from_GLenum(const GLenum gl_type) {

    switch (gl_type) {
        case GL_UNSIGNED_BYTE:
            return torch::kUInt8;
        case GL_BYTE:
            return torch::kInt8;
        case GL_UNSIGNED_SHORT:
            std::cerr << "Using kInt16 for GL_UNSIGNED_SHORT. This may lead to errors." << std::endl;
            return torch::kInt16;
        case GL_SHORT:
            return torch::kInt16;
        case GL_UNSIGNED_INT:
            std::cerr << "Using kInt32 for GL_UNSIGNED_INT. This may lead to errors." << std::endl;
            return torch::kInt32;
        case GL_INT:
            return torch::kInt32;
        case GL_HALF_FLOAT:
            return torch::kFloat16;
        case GL_FLOAT:
            return torch::kFloat32;

        default:
            // TODO: Print name?
            std::cerr << "Warning: Texture uses unknown type with id " << gl_type << std::endl;
            std::cerr << "Using kFloat32 as type. This may lead to errors." << std::endl;
            return torch::kFloat32;
    }

    //   return std::make_pair(d_t, type_size);
}

Layout::Layout(const std::string &layout_str, bool h_up) : layout_str(layout_str), h_up(h_up) {}

static int get_index(char c, const std::string &l)
{
    int i = l.find(c);
    if (i == l.length())
        i = -1;
    return i;
}

at::Tensor Layout::convert(at::Tensor &tensor, const Layout &from, const Layout &to)
{
    std::string current = from.layout_str;
    if (from.layout_str.length() < to.layout_str.length())
    {
        // we need to unsqueeze
        for (uint32_t i = 0; i < to.layout_str.length(); ++i)
        {
            char to_name = to.layout_str[i];
            if (get_index(to_name, from.layout_str) < 0)
            {
                // dim not in from --> unsqueeze
                current = to_name + current;
                tensor.unsqueeze_(0);
            }
        }
    }
    else
    {
        // we need to squeeze
        for (uint32_t i = 0; i < from.layout_str.length(); ++i)
        {
            char from_name = from.layout_str[i];
            if (get_index(from_name, to.layout_str) < 0)
            {
                if (tensor.size(i) != 1)
                {
                    std::cerr << from_name
                              << "has dim != 1, thus cannot be squozen!"
                              << std::endl;
                }
                current.erase(std::remove(current.begin(), current.end(), from_name), current.end());
                tensor.squeeze_(i);
            }
        }
    }
    // check whether we expanded correctly
    if (to.layout_str.length() != current.length())
        std::cerr << "internal error: " << to.layout_str.length()
                  << " does not match " << current.length() << std::endl;

    // permute
    std::vector<int64_t> dim_indices;
    for (const auto &c : to.layout_str)
    {
        int cur_index = get_index(c, current);
        dim_indices.push_back(cur_index);
        // flip h dim
        if (c == 'h' && from.h_up != to.h_up)
        {
            tensor = tensor.flip({cur_index});
        }
    }

    if (dim_indices.size() != tensor.dim())
    {
        std::cerr << "Mismatching dims for permuting..." << std::endl;
        std::cerr << "tensor with " << tensor.dim() << " dimensions"
                  << "(" << tensor.sizes() << ")"
                  << " cannot be permuted by " << dim_indices << std::endl;
    }

    tensor = tensor.permute(at::IntArrayRef(dim_indices));

    return tensor;
}

at::Tensor Layout::to(at::Tensor &t, const Layout &to) const
{
    return Layout::convert(t, *this, to);
}

void map_value_range(at::Tensor &tensor, const c10::ArrayRef<double> &from, const c10::ArrayRef<double> &to)
{
    tensor -= from[0];
    tensor /= (from[1] - from[0]);

    tensor *= (to[1] - to[0]);
    tensor += to[0];
}

void print_tensor_info(const at::Tensor &t, bool print_data)
{
    // general info
    std::cout << t.sizes() << std::endl;
    std::cout << t.options() << std::endl;

    // min max
    auto mi = t.min().item().toFloat();
    auto ma = t.max().item().toFloat();
    std::cout << "min: " << mi << ", max: " << ma << std::endl;

    // var mean
    if (t.dtype() == at::kFloat)
        std::cout << "var, mean: " << t.var().item().toFloat() << ", "
                  << t.mean().item().toFloat() << std::endl;

    // data
    if (print_data)
        std::cout << "data: " << std::endl
                  << t << std::endl;
}

void print_model_info(torch::nn::Module model)
{
    for (const auto &p : model.named_parameters())
        std::cout << p->name() << " - " << p.key() << std::endl;

    for (const auto &p : model.parameters())
        std::cout << p << std::endl;
}

at::Tensor textureToTensor(cppgl::Texture2D tex, bool normalize)
{
    if (tex->type == GL_FLOAT)
    {
        std::vector<float> pixels;
        return textureToTensor_body(tex, pixels, normalize);
    }
    else if (tex->type == GL_UNSIGNED_BYTE)
    {
        std::vector<uint8_t> pixels;
        return textureToTensor_body(tex, pixels, normalize);
    }
    else
    {
        std::cout << "invalid type" << std::endl;
        return at::Tensor();
    }
}

torch::jit::script::Module load_module(const fs::path &module_path, const torch::DeviceType device)
{
    if(module_path.empty()) {
        std::cerr << "load_module(): path is empty!" << std::endl;
        exit(-1);
    }

    if(! fs::exists(module_path)) {
        std::cerr << "load_module(): " << module_path << " does not exsist!" << std::endl;
        exit(-1);
    }

    torch::jit::script::Module module;
    try
    {
        module = torch::jit::load(module_path.string());
        module.to(device_str);
        module.eval();
    }
    catch (const c10::Error &e)
    {
        std::cerr << e.msg();
        exit(-1);
    }
    return module;
}

void screenshot_tensor_to_path(const torch::Tensor &tensor, const fs::path &path, int i)
{
    auto sizes = tensor.sizes();
    if (sizes.size() != 3)
    {
        std::cerr << "screenshot: sizes invalid" << std::endl;
    }
    if (sizes[0] != 3)
    {
        std::cerr << "screenshot: need to have 3 color channels" << std::endl;
        return;
    }
    auto ten_c = tensor.to(torch::kCPU).clone();
    ten_c = ten_c.permute({1, 2, 0}).contiguous();
    std::vector<float> arr(sizes[0] * sizes[1] * sizes[2]);
    std::vector<uint8_t> arr_char(sizes[0] * sizes[1] * sizes[2]);
    std::memcpy(arr.data(), ten_c.data_ptr(), sizes[0] * sizes[1] * sizes[2] * sizeof(float));
    for (int i = 0; i < arr.size(); ++i)
    {
        arr_char[i] = uint8_t(arr[i] * 255.f);
    }
    stbi_write_png(path.c_str(), sizes[2], sizes[1], 3, arr_char.data(), 0);
}

void screenshot_tensor_to_path(const torch::Tensor &tensor, const fs::path &path, const Layout &in_layout, const c10::ArrayRef<double> &value_range)
{
    auto t = tensor.clone();
    if (t.dtype() == c10::kFloat)
    {
        map_value_range(t, value_range, {0., 255.});
        t = t.clamp(0.f, 255.f).to(c10::kByte);
    }
    auto ten_c = t.to(torch::kCPU);
    ten_c = in_layout.to(ten_c, STBI_LAYOUT).contiguous();
    // ten_c = ten_c.permute({ 2,1,0 }).contiguous();
    auto sizes = ten_c.sizes();
    if (sizes[2] != 3)
    {
        std::cerr << "screenshot: need to have 3 color channels" << std::endl;
        return;
    }
    auto len = sizes[0] * sizes[1] * sizes[2];
    std::vector<uint8_t> arr(len);
    std::memcpy(arr.data(), ten_c.data_ptr(), len * sizeof(uint8_t));

    stbi_write_png(path.c_str(), sizes[1], sizes[0], 3, arr.data(), 0);
}

void sync_cuda()
{
    static at::cuda::CUDAStream stream = at::cuda::getCurrentCUDAStream();
    AT_CUDA_CHECK(cudaStreamSynchronize(stream));
}
