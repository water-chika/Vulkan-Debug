cmake_minimum_required(VERSION 3.20)

project(Vulkan-Debug)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

find_package(Vulkan REQUIRED)

add_custom_command(OUTPUT comp.spv
  COMMAND Vulkan::glslangValidator --target-env vulkan1.3
              ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.comp Vulkan::glslangValidator)

add_executable(compute_shader_debug main.cpp comp.spv vulkan_helper.hpp spirv_helper.hpp)
target_link_libraries(compute_shader_debug Vulkan::Vulkan)

add_executable(compute_shader_debug_c main.c comp.spv)
target_link_libraries(compute_shader_debug_c Vulkan::Vulkan)

add_executable(graphics_pipeline_debug graphics_pipeline_debug.cpp vulkan_helper.hpp spirv_helper.hpp)
target_link_libraries(graphics_pipeline_debug Vulkan::Vulkan)

add_executable(enum_to_string enum_to_string.cpp)
