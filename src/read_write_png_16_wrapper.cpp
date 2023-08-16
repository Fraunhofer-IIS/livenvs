#include "read_write_png_16_wrapper.h"
#include <iostream>
// #include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <algorithm>


std::vector<uint16_t> read_png_16(const std::string& filename, uint32_t& w, uint32_t& h){
    uint32_t c = 1; // we only allow grayscale
    int i =0;
    FILE *fp = fopen(filename.c_str(), "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        std::cout << "!ong" << std::endl;
        exit(0);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cout << "!info" << std::endl;
        exit(0);
    }

    if (setjmp(png_jmpbuf(png))) {
        std::cout << "etjmp(png_jmpbuf(png)" << std::endl;
        exit(0);
    }

    png_init_io(png, fp);

    png_read_info(png, info);

    w = png_get_image_width(png, info);
    h = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if (bit_depth != 16){
        std::cout << "bit depth not 16!" << std::endl;
        exit(0);
    }
        
    // if (color_type == PNG_COLOR_TYPE_PALETTE)
    //     png_set_palette_to_rgb(png);

    // // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    // if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    //     png_set_expand_gray_1_2_4_to_8(png);

    // if (png_get_valid(png, info, PNG_INFO_tRNS))
    //     png_set_tRNS_to_alpha(png);

    // // These color_type don't have an alpha channel then fill it with 0xff.
    // if (color_type == PNG_COLOR_TYPE_RGB ||
    //     color_type == PNG_COLOR_TYPE_GRAY ||
    //     color_type == PNG_COLOR_TYPE_PALETTE)
    //     png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    // if (color_type == PNG_COLOR_TYPE_GRAY ||
    //     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    //     png_set_gray_to_rgb(png);

      // from https://stackoverflow.com/questions/51922475/how-to-write-a-16-bit-png-color-type-gray-with-libpng
    png_set_swap(png); // <--- THIS LINE ADDED

    png_read_update_info(png, info);

    std::vector<uint16_t> image(w*h*c);
    for (int y = h-1; y >= 0; y--)
    {
        png_bytep row = (png_byte *)malloc(png_get_rowbytes(png, info));
        png_read_row(png, row, NULL);
        std::copy(&((uint16_t*)row)[0], &((uint16_t*)row)[w], &image[y*w]);
        free(row);
    }

    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);

    return image;
}

void write_png_16(const std::string& filename, const std::vector<uint16_t>& image, uint32_t w, uint32_t h){
    const uint32_t c = 1; // we only allow grayscale

    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp)
        abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        abort();

    png_infop info = png_create_info_struct(png);
    if (!info)
        abort();

    if (setjmp(png_jmpbuf(png)))
        abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        w, h,
        16,
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // from https://stackoverflow.com/questions/51922475/how-to-write-a-16-bit-png-color-type-gray-with-libpng
    png_set_swap(png); // <--- THIS LINE ADDED

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (image.empty())
        abort();

    for (int y = h-1; y >= 0; y--)
        png_write_row(png, (png_const_bytep)&image[y * w * c]);
    png_write_end(png, NULL);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
}

void test_wrapper(){
           
            std::vector<uint16_t> image;
            std::string out_file = "./pngnewtest.png";
            for (uint32_t y =0; y < 480; y++){
                for (uint32_t x =0; x < 640; x++){
                    image.push_back(100*x);
                }
            }
            write_png_16(out_file, image, 640, 480);

            uint32_t w,h;
           read_png_16(out_file, w,h);
}