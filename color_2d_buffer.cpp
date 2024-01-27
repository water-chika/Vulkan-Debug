#include <cstdint>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>

struct uint8 {
    uint8_t value;
};

std::ostream& operator<<(std::ostream& out, uint8 v) {
    return out << static_cast<int>(v.value);
}

struct rgba {
    uint8 r,g,b,a;
};

template<class T>
void print_range(std::ostream& out, T range, std::string div) {
    for (int i = 0; i + 1 < range.size(); i++) {
        out << range[i] << div;
    }
    if (range.size() > 0)
        out << range[range.size() - 1];
}

std::ostream& operator<<(std::ostream& out, rgba& color) {
    std::array<uint8, 4> components{ color.r, color.g, color.b, color.a };
    print_range(out, components, ", ");
    return out;
}

class color_2d_buffer {
    class column_ref {
    public:
        column_ref(rgba* pixels, int stride, int x) : pixels{ pixels }, stride{ stride }, x{ x } { }
        rgba& operator[](int y) {
            return pixels[x + y*stride];
        }
    private:
        rgba* pixels;
        int stride;
        int x;
    };
public:
    color_2d_buffer(int width, int height) : width{width}, height{height}, pixels(width*height){}
    column_ref operator[](int x) {
        return column_ref{pixels.data(), width, x};
    }
private:
    std::vector<rgba> pixels;
    int width;
    int height;
};

int main() {
    color_2d_buffer image{128,128};
    image[0][1] = rgba{128,0,0,128};
    std::cout << image[0][1] << std::endl;
}
