cmake_minimum_required(VERSION 3.20)

project(Vulkan-Debug)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

add_subdirectory(vulkan_helper)
find_package(Vulkan REQUIRED)

add_custom_command(OUTPUT comp.spv
  COMMAND Vulkan::glslangValidator --target-env vulkan1.3
              ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.comp Vulkan::glslangValidator)

add_executable(compute_shader_debug compute.cpp comp.spv)
target_link_libraries(compute_shader_debug vulkan_helper)

add_executable(compute_shader_debug_c compute.c comp.spv)
target_link_libraries(compute_shader_debug_c Vulkan::Vulkan)
