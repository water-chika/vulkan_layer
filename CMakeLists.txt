cmake_minimum_required(VERSION 3.21)

project(vk_water_chika_debug_layer)

find_package(Vulkan REQUIRED)

add_library(vk_water_chika_debug_layer SHARED
    vk_water_chika_debug_layer.cpp
    vk_water_chika_debug_layer.hpp
)

target_include_directories(vk_water_chika_debug_layer PUBLIC ${Vulkan_INCLUDE_DIRS})

set_target_properties(vk_water_chika_debug_layer PROPERTIES CXX_STANDARD 23)

add_executable(vk_layer_generator
    vk_layer_generator.cpp
    vk_layer_generator.hpp
)

target_include_directories(vk_layer_generator PUBLIC ${Vulkan_INCLUDE_DIRS})
set_target_properties(vk_layer_generator PROPERTIES CXX_STANDARD 23)
