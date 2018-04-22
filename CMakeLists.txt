cmake_minimum_required(VERSION 3.1)

project(LavaDemos)

add_subdirectory(extern/glfw)
add_subdirectory(extern/glslang)

link_libraries(glfw glslang SPIRV)

set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++")

set(SOURCE
    src/LavaCompiler.cpp)

add_executable(vtriangle vtriangle.cpp ${SOURCE})

target_include_directories(vtriangle PRIVATE
    "${VULKAN_INCLUDE_DIR}"
    extern/spdlog/include
    include)

target_link_libraries(vtriangle
    "${VULKAN_LIBRARY}")
