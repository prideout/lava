cmake_minimum_required(VERSION 3.0)

project(LavaDemos C)

add_subdirectory(extern/glfw)

link_libraries(glfw)

add_executable(vtriangle vtriangle.c)

target_include_directories(vtriangle PRIVATE
    "${VULKAN_INCLUDE_DIR}"
)

target_link_libraries(vtriangle
    "${VULKAN_LIBRARY}"
)
