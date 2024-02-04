cmake_minimum_required(VERSION 3.23)

project(opengl_vram_size)

add_subdirectory(glfw)

add_executable(opengl_vram_size opengl_vram_size.cpp glad/src/gl.c glad/src/wgl.c)
target_link_libraries(opengl_vram_size PUBLIC glfw)
target_include_directories(opengl_vram_size PUBLIC glad/include)