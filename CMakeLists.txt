cmake_minimum_required(VERSION 3.20)

project(OpenGL)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

add_executable(opengl opengl.cpp)
target_include_directories(opengl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

