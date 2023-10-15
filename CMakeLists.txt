cmake_minimum_required(VERSION 3.20)

project(Vulkan-Debug)

find_package(Vulkan REQUIRED)

add_custom_command(OUTPUT comp.spv
  COMMAND Vulkan::glslangValidator --target-env vulkan1.3
              ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.comp Vulkan::glslangValidator)

add_executable(compute_shader_debug main.cpp comp.spv)
target_link_libraries(compute_shader_debug Vulkan::Vulkan)

add_executable(compute_shader_debug_c main.c comp.spv)
target_link_libraries(compute_shader_debug_c Vulkan::Vulkan)
