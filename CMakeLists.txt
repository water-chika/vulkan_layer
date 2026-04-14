cmake_minimum_required(VERSION 3.21)

project(vk_water_chika_debug_layer)

add_library(vk_water_chika_debug_layer SHARED
    vk_water_chika_debug_layer.cpp
    vk_water_chika_debug_layer.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/vk_cmd_buf_split_info.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/device_dispatch.hpp
)

target_include_directories(vk_water_chika_debug_layer PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(vk_water_chika_debug_layer PUBLIC
    spirv_parser
    Vulkan::Headers
)

target_compile_features(vk_water_chika_debug_layer PUBLIC cxx_std_23)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vk_cmd_buf_split_info.hpp
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generate_cmd_buf_layer.py ${vulkan_registry_xml} --output ${CMAKE_CURRENT_BINARY_DIR}/vk_cmd_buf_split_info.hpp
    DEPENDS ${vulkan_registry_xml} ${CMAKE_CURRENT_SOURCE_DIR}/generate_cmd_buf_layer.py
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/device_dispatch.hpp
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generate_device_dispatch.py ${vulkan_registry_xml} --output ${CMAKE_CURRENT_BINARY_DIR}/device_dispatch.hpp
    DEPENDS ${vulkan_registry_xml} ${CMAKE_CURRENT_SOURCE_DIR}/generate_device_dispatch.py
)


add_executable(vk_layer_generator
    vk_layer_generator.cpp
    vk_layer_generator.hpp
)

target_include_directories(vk_layer_generator PUBLIC ${Vulkan_INCLUDE_DIRS})
target_compile_features(vk_layer_generator PUBLIC cxx_std_23)

