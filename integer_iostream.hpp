#pragma once
#include <iostream>

template<int N>
std::ostream& operator<<(std::ostream& out, int_<N> number) {
    static_assert(N > 8);
    return out << number.value;
}

std::ostream& operator<<(std::ostream& out, int_<8> number) {
    return out << static_cast<int32_t>(number.value);
}