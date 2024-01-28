cmake_minimum_required(VERSION 3.20)

project(color_2d_buffer)

add_executable(test test.cpp color_2d_buffer.hpp)
set_property(TARGET test PROPERTY CXX_STANDARD 23)
