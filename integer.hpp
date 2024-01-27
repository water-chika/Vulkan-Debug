#pragma once

#include<cstdint>

template<int N>
struct int_ {
    int value;
};

template<>
struct int_<8> {
    int8_t value;
};

template<>
struct int_<16> {
    int16_t value;
};

template<>
struct int_<32> {
    int32_t value;
};

template<>
struct int_<64> {
    int64_t value;
};