#include <cstdint>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>

struct uint8 {
    uint8_t value;
};
using uint16 = uint16_t;
using uint32 = uint32_t;
using float32 = float;

std::ostream& operator<<(std::ostream& out, uint8 v) {
    return out << static_cast<int>(v.value);
}

namespace format {
    struct r8 {
        uint8 r;
    };
    struct r16 {
        uint16 r;
    };
    struct rg8 {
        uint8 r,g;
    };
    struct rg16 {
        uint16 r,g;
    };
    struct rgb8 {
        uint8 r,g,b;
    };
    struct rgba8 {
        uint8 r,g,b,a;
    };
    struct r32f {
        float32 r;
    };
    struct rg32f {
        float32 r,g;
    };
    struct rgb32f {
        float32 r,g,b;
    };
    struct rgba32f {
        float32 r,g,b,a;
    };
}

template<class T>
void print_range(std::ostream& out, T range, std::string div) {
    for (int i = 0; i + 1 < range.size(); i++) {
        out << range[i] << div;
    }
    if (range.size() > 0)
        out << range[range.size() - 1];
}

std::ostream& operator<<(std::ostream& out, format::rgba8& color) {
    std::array<uint8, 4> components{ color.r, color.g, color.b, color.a };
    print_range(out, components, ", ");
    return out;
}

template<class T>
class color_2d_buffer {
    using pixel_t = T;
    class column_ref {
    public:
        column_ref(pixel_t* pixels, int stride, int x) : pixels{ pixels }, stride{ stride }, x{ x } { }
        pixel_t& operator[](int y) {
            return pixels[x + y*stride];
        }
    private:
        pixel_t* pixels;
        int stride;
        int x;
    };
public:
    color_2d_buffer(int width, int height) : width{width}, height{height}, pixels(width*height){}
    column_ref operator[](int x) {
        return column_ref{pixels.data(), width, x};
    }
private:
    std::vector<pixel_t> pixels;
    int width;
    int height;
};

