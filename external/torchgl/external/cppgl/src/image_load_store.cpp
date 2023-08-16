#include "image_load_store.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stbi/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi/stb_image_write.h"
#include <algorithm>
#include <thread>

CPPGL_NAMESPACE_BEGIN

///////////////////////
//load

std::tuple<std::vector<uint8_t>, int, int, int, bool> image_load(const std::filesystem::path& path) {
    stbi_set_flip_vertically_on_load(1); // important: the default value for this is different on windows and linux

    uint8_t* data = 0;
    int w_out, h_out, channels_out;
    bool is_hdr_out = stbi_is_hdr(path.string().c_str());
    size_t size_of_buffer = 0;

    // load image from disk
    if (is_hdr_out) {
        data = (uint8_t*)stbi_loadf(path.string().c_str(), &w_out, &h_out, &channels_out, 0);
        size_of_buffer = w_out * h_out * channels_out * sizeof(float);
    } else {
        data = stbi_load(path.string().c_str(), &w_out, &h_out, &channels_out, 0);
        size_of_buffer = w_out * h_out * channels_out;
    }
    if (!data)
        throw std::runtime_error("Failed to load image file: " + path.string());

    // copy array to output
    std::vector<uint8_t> data_out;
    data_out.insert(data_out.end(), &data[0], &data[size_of_buffer]);
    
    // free data
    stbi_image_free(data);

    return { data_out, w_out, h_out, channels_out, is_hdr_out };
}

///////////////////////
//save

void image_store_ldr_impl(const std::filesystem::path& path, const uint8_t* image_data, int w, int h, int channels, bool flip) {
    stbi_flip_vertically_on_write(flip);

    if (path.extension() == ".png")
        stbi_write_png(path.string().c_str(), w, h, channels, image_data, 0); // stride 0, tightly packed
    else if (path.extension() == ".jpg" || path.extension() == ".jpeg")
        stbi_write_jpg(path.string().c_str(), w, h, channels, image_data, 100); // quality fixed at 100%
    else if (path.extension() == ".tga")
        stbi_write_tga(path.string().c_str(), w, h, channels, image_data);
    else if (path.extension() == ".bmp")
        stbi_write_bmp(path.string().c_str(), w, h, channels, image_data);
    else
        throw std::runtime_error("save_image_ldr: unsupported image format: " + path.extension().string());
}

void image_store_ldr_thread(const std::filesystem::path& path, const std::shared_ptr<std::vector<uint8_t>>& image_data, int w, int h, int channels, bool flip) {
    image_store_ldr_impl(path, image_data->data(), w, h, channels, flip);
}

void image_store_ldr(const std::filesystem::path& path, const uint8_t* image_data, int w, int h, int channels, bool flip, bool async) {
    if (async) {
        std::shared_ptr<std::vector<uint8_t>> image_data_vector = std::make_shared<std::vector<uint8_t>>(image_data, image_data + size_t(w) * h * channels);
        std::thread worker(image_store_ldr_thread, path, image_data_vector, w, h, channels, flip);
        worker.detach();
    } else
        image_store_ldr_impl(path, image_data, w, h, channels, flip);
}

void image_store_hdr_impl(const std::filesystem::path& path, const float* image_data, int w, int h, int channels, bool flip) {
    stbi_flip_vertically_on_write(flip);

    if (path.extension() == ".hdr")
        stbi_write_hdr(path.string().c_str(), w, h, channels, image_data);
    else
        throw std::runtime_error("save_image_hdr: unsupported image format: " + path.extension().string());
}

void image_store_hdr_thread(const std::filesystem::path& path, const std::shared_ptr<std::vector<float>>& image_data, int w, int h, int channels, bool flip) {
    image_store_hdr_impl(path, image_data->data(), w, h, channels, flip);
}

void image_store_hdr(const std::filesystem::path& path, const float* image_data, int w, int h, int channels, bool flip, bool async) {
    if (async) {
        std::shared_ptr<std::vector<float>> image_data_vector = std::make_shared<std::vector<float>>(image_data, image_data + size_t(w) * h * channels);
        std::thread worker(image_store_hdr_thread, path, image_data_vector, w, h, channels, flip);
        worker.detach();
    } else
        image_store_hdr_impl(path, image_data, w, h, channels, flip);
}

CPPGL_NAMESPACE_END
