#include "write_out.h"
#include "float16.h"
#include "../tiny_image_io.h"

// // note the call by reference!
// std::vector<uint8_t> crop_to_byte(const std::vector<float>& in, const uint32_t channels, const uint32_t in_w, const uint32_t in_h, uint32_t& out_w, uint32_t& out_h){
//     if( in_h < out_h || in_w < out_w){
//         out_h = in_h;
//         out_w = in_w;
//     }
//     std::vector<uint8_t> out = std::vector<uint8_t>(out_w*out_h*channels);
//     for (uint32_t y = 0; y < out_h; ++y){
//         for (uint32_t x = 0; x < out_w; ++x){
//             for (uint32_t c = 0; c < channels; ++c){
//                 uint32_t linear_index_out = (y * out_w + x) *channels + c;
//                 uint32_t linear_index_in = (y * in_w + x) *channels + c;
//                 out[linear_index_out] = uint8_t(255.f*in[linear_index_in]);
//             }
//         }
//     }
//     return out;
// } 

// // note the call by reference!
// std::vector<float> crop(const std::vector<float>& in, const uint32_t channels, const uint32_t in_w, const uint32_t in_h, uint32_t& out_w, uint32_t& out_h){
//     if( in_h < out_h || in_w < out_w){
//         out_h = in_h;
//         out_w = in_w;
//     }
//     std::vector<float> out = std::vector<float>(out_w*out_h*channels);
//     for (uint32_t y = 0; y < out_h; ++y){
//         for (uint32_t x = 0; x < out_w; ++x){
//             for (uint32_t c = 0; c < channels; ++c){
//                 uint32_t linear_index_out = (y * out_w + x) *channels + c;
//                 uint32_t linear_index_in = (y * in_w + x) *channels + c;
//                 out[linear_index_out] = in[linear_index_in];
//             }
//         }
//     }
//     return out;
// } 

// // note the call by reference!
// std::vector<char> crop(const std::vector<char>& in, const uint32_t channels, const uint32_t in_w, const uint32_t in_h, uint32_t& out_w, uint32_t& out_h){
//     if( in_h < out_h || in_w < out_w){
//         out_h = in_h;
//         out_w = in_w;
//     }

//     std::vector<char> out = std::vector<char>(out_w*out_h*channels);
//     for (uint32_t y = 0; y < out_h; ++y){
//         for (uint32_t x = 0; x < out_w; ++x){
//             for (uint32_t c = 0; c < channels; ++c){
//                 uint32_t linear_index_out = (y * out_w + x) *channels + c;
//                 uint32_t linear_index_in = (y * in_w + x) *channels + c;
//                 out[linear_index_out] = in[linear_index_in];
//             }
//         }
//     }
//     return out;
// } 

// void export_utils::write_as_jpg   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float> data, bool flip){ 
//     // XXX data is float right now!!!! TODO cast!!!
//     stbi_flip_vertically_on_write(true);
//     stbi_write_jpg(filename.c_str(), w, h, channels, data.data(), 100);
// }
// void export_utils::write_as_png   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float> data, bool flip){
//     // XXX data is float right now!!!! TODO cast!!!
//     stbi_flip_vertically_on_write(true);
//     stbi_write_png(filename.c_str(), w, h, channels, data.data(), 100);
// }


void export_utils::write_as_jpg   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<uint8_t>& data, bool flip){ 
    std::cout << filename.c_str() << std::endl;
    assert(w*h*channels == data.size());
    assert(w*h*channels > 0);
    if (w*h*channels == data.size()){
        std::cout << "good" << std::endl;
    }
    std::cout << w << " " << h << " " << channels  << " " << data.size() << std::endl;
    assert(!filename.empty());
    stbi_flip_vertically_on_write(true);

    int i = 3434;
    std:: cout << data[i] << std::endl;

    stbi_write_jpg(filename.c_str(), w, h, channels, data.data(), 100);
}
void export_utils::write_as_png   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<uint8_t>& data, bool flip){ 
    assert(!filename.empty());
    assert(w*h*channels == data.size());
    assert(w*h*channels > 0);
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filename.c_str(), w, h, channels, data.data(), 0);
}


void export_utils::write_as_jpg   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float>& data, bool flip){ 
    std::cout << filename.c_str() << std::endl;
    assert(w*h*channels == data.size());
    assert(w*h*channels > 0);
    if (w*h*channels == data.size()){
        std::cout << "good" << std::endl;
    }
    std::cout << w << " " << h << " " << channels  << " " << data.size() << std::endl;
    assert(!filename.empty());
    std::vector<uint8_t> casted = std::vector<uint8_t>(data.size());
    for (uint32_t i = 0; i < data.size(); ++i) casted[i] = uint8_t(255.f*data[i]);
    stbi_flip_vertically_on_write(true);
    stbi_write_jpg(filename.c_str(), w, h, channels, casted.data(), 100);
}
void export_utils::write_as_png   (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const std::vector<float>& data, bool flip){
    assert(!filename.empty());
    assert(w*h*channels == data.size());
    assert(w*h*channels > 0);
    std::vector<uint8_t> casted = std::vector<uint8_t>(data.size());
    for (uint32_t i = 0; i < data.size(); ++i) casted[i] = uint8_t(255.f*data[i]);
    stbi_flip_vertically_on_write(true);
    stbi_write_png(filename.c_str(), w, h, channels, casted.data(), 0);
}

/// WARNING: this does not check or save the endian type!!
/// write blob with layout:
///     width height channels bytes_per_channel 0 data
void export_utils::write_as_blob  (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const uint32_t bytes_per_channel, const std::vector<float> data, bool flip){
    if(w*h*channels != data.size()){
        std::cerr << "Mismatching dimensions: " << w*h*channels << " != " << data.size() << std::endl;
        return;
    }
    auto blobfile = std::fstream(filename.c_str(), std::ios::out | std::ios::binary);
    if(!blobfile){
        std::cerr << "Could not open " << filename<< std::endl;
        return;
    }

    static const uint32_t null_mark = 0;
    // write meta information: width height channels 0
    blobfile.write((char*)&w, sizeof(uint32_t));
    blobfile.write((char*)&h, sizeof(uint32_t));
    blobfile.write((char*)&channels, sizeof(uint32_t));
    blobfile.write((char*)&bytes_per_channel, sizeof(uint32_t));
    blobfile.write((char*)&null_mark, sizeof(uint32_t));

    char* casted_data;
    bool delete_casted_data = false;
    
    // cast to match bytes_per_channel
    switch (bytes_per_channel)
    {
    case 4:   // float, do nothing
        casted_data = (char*)&data[0];
        break;
    case 2: { // half / float16 cast
            casted_data = new char[w*h*channels*bytes_per_channel];
            delete_casted_data = true;
            uint16_t* it = (uint16_t*)casted_data;
            for (const auto& element : data){
                auto half_flt = quick_encode_flt16(element);
                *it = half_flt;
                //std::cout << element << " == " << decode_flt16<float>(*it) << "?" << std::endl;
                ++it;
            } 
        }
        break;
    case 1: { // uint8_t
            casted_data = new char[w*h*channels*bytes_per_channel];
            delete_casted_data = true;
            char* it = casted_data;
            for (const auto& element : data){
                auto half_flt = uint8_t(255.f*element);
                *it = half_flt;
                ++it;
            }         
            std::cout << data[1] << " == " << casted_data[1]/255.f << "?" << std::endl;
        }
        break;
    default:
        std::cerr << "not implemented bytes per channel!" << std::endl;
        break;
    }

    // write blob
    if(flip){
        for (uint32_t y = h-1; y < h; --y)
            blobfile.write((char*)&casted_data[y*w*channels*bytes_per_channel], w*channels*bytes_per_channel);
    } else
        blobfile.write(casted_data, w*h*channels*bytes_per_channel);
    blobfile.close();

    if (delete_casted_data) delete[] casted_data;
}

// XXX TODO XXX
// void export_utils::write_as_tiff  (const fs::path& filename, const uint32_t w, const uint32_t h, const uint32_t channels, const uint32_t bytes_per_channel, const std::vector<float> data, bool flip)


