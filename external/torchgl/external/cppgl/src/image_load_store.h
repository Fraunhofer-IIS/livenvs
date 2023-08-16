#pragma once
#include <vector>
#include <filesystem>
#include "platform.h"

CPPGL_NAMESPACE_BEGIN

// Return values: image data, width, height, channels, is_hdr
// Usage: auto [data, w, h, c, is_hdr] = load_image(path);
// Note: if is_hdr is set, image data is of type float stored as byte array
std::tuple<std::vector<uint8_t>, int, int, int, bool> image_load(const std::filesystem::path& path);

// Write LDR image to disk, supported file formats: .png, .jpg/.jpeg, .tga, .bmp
void image_store_ldr(const std::filesystem::path& path, const uint8_t* image_data, int w, int h, int channels, bool flip = true, bool async = false);

// Write HDR image to disk, supported file formats: .hdr
void image_store_hdr(const std::filesystem::path& path, const float* image_data, int w, int h, int channels, bool flip = true, bool async = false);

CPPGL_NAMESPACE_END
