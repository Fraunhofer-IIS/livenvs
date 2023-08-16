# pragma once
#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>
#include <fstream>
#include <vector>

namespace export_utils {
    template< typename T_o , typename T_i >
    std::vector<T_o> crop(const std::vector<T_i>& in, const uint32_t channels, const uint32_t in_w, const uint32_t in_h, uint32_t& out_w, uint32_t& out_h){
        static_assert(std::is_same<T_o, float>() || std::is_same<T_o, uint8_t>(), "T_o of bad type bruh");
        static_assert(std::is_same<T_i, float>() || std::is_same<T_i, uint8_t>(), "T_i of bad type bruh");
        if( in_h < out_h || in_w < out_w){
            std::cerr << "WARNING: out_w or out_h are out of bounds!... are set to in_w and out_w!" << std::endl;
            out_h = in_h;
            out_w = in_w;
        }

        std::vector<T_o> out = std::vector<T_o>(out_w*out_h*channels);
        for (uint32_t y = 0; y < out_h; ++y){
            for (uint32_t x = 0; x < out_w; ++x){
                for (uint32_t c = 0; c < channels; ++c){
                    uint32_t linear_index_out = (y * out_w + x) *channels + c;
                    uint32_t linear_index_in = (y * in_w + x) *channels + c;
                    
                    if constexpr(std::is_same<T_o, uint8_t>() && std::is_same<T_i, float>()){
                        out[linear_index_out] = uint8_t(255.f*in[linear_index_in]);
                    }
                    else if constexpr(std::is_same<T_i, float>() && std::is_same<T_o, float>())
                        out[linear_index_out] = in[linear_index_in];
                    else if constexpr(std::is_same<T_i, uint8_t>() && std::is_same<T_o, uint8_t>())
                        out[linear_index_out] = in[linear_index_in];
                    else if constexpr(std::is_same<T_i, uint8_t>() && std::is_same<T_o, float>())
                        out[linear_index_out] = float(in[linear_index_in])/255.f;
                }
            }
        }
        return out;
    }
    
    void write_as_jpg   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float>& data, bool flip = true);
    void write_as_png   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float>& data, bool flip = true);
    void write_as_jpg   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<uint8_t>& data, bool flip = true);
    void write_as_png   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<uint8_t>& data, bool flip = true);
    /// WARNING: this does not check or save the endian type!!
    /// write blob with layout:
    ///     width height channels bytes_per_channel 0 data
    void write_as_blob  (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const uint32_t bytes_per_channel, const std::vector<float> data, bool flip = true);
    void write_as_tiff  (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const uint32_t bytes_per_channel, const std::vector<float> data, bool flip = true);
}