cmake_minimum_required(VERSION 3.1)

project(LavaDemos)

add_subdirectory(extern/glfw)
add_subdirectory(extern/glslang)

link_libraries(glfw glslang SPIRV)

set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++")

add_executable(vtriangle vtriangle.cpp)

target_include_directories(vtriangle PRIVATE
    "${VULKAN_INCLUDE_DIR}"
)

target_link_libraries(vtriangle
    "${VULKAN_LIBRARY}"
)
