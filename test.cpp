#include "integer.hpp"
#include "integer_iostream.hpp"

#include <iostream>

int main() {
    int_<8> i{ 52 };
    std::cout << i << std::endl;

    int8_t j{ 52 };
    std::cout << j << std::endl;
    return 0;
}