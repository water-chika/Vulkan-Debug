cmake_minimum_required(VERSION 3.20)

project(cxx_integer)

add_executable(test test.cpp integer.hpp integer_iostream.hpp)
set_property(TARGET test PROPERTY CXX_STANDARD 23)