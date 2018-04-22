cmake_minimum_required(VERSION 3.1)

project(LavaDemos)

add_subdirectory(extern/glfw)
add_subdirectory(extern/glslang)

link_libraries(
    glfw
    glslang
    SPIRV
    "${VULKAN_LIBRARY}")

include_directories(${DEMO} PRIVATE
    "${VULKAN_INCLUDE_DIR}"
    extern/spdlog/include
    include)

set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++")

file(GLOB_RECURSE LAVA_SOURCE "src/Lava*.cpp")

set(DEMO_NAMES
    vtriangle
    vertexid)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} demos/${DEMO}.cpp ${LAVA_SOURCE})
endforeach()
