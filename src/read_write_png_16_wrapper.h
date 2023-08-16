#include <string>
#include <type_traits>
#include <vector>

// read 16-bit grascale image
std::vector<uint16_t> read_png_16(const std::string& filename, uint32_t& w, uint32_t& h);

// write 16-bit grascale image
void write_png_16(const std::string& filename, const std::vector<uint16_t>& image, uint32_t w, uint32_t h);

void test_wrapper();