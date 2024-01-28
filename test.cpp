#include "color_2d_buffer.hpp"

int main() {
    color_2d_buffer<format::rgba8> image{128,128};
    image[0][1] = format::rgba8{128,0,0,128};
    std::cout << image[0][1] << std::endl;
}
