cmake_minimum_required(VERSION 3.20)

project(OpenGL)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

add_executable(test test.cpp opengl.cpp)
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

