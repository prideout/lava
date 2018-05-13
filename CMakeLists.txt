cmake_minimum_required(VERSION 3.1)

project(LavaDemos)

add_subdirectory(extern/glfw)
add_subdirectory(extern/glslang)

link_libraries(
    glfw
    glslang
    SPIRV)

include_directories(${DEMO} PRIVATE
    extern/spdlog/include
    extern/vkload/include
    extern/glslang
    include)

set(CMAKE_CXX_FLAGS "-std=c++17 -stdlib=libc++ -fno-rtti")

file(GLOB_RECURSE LAVA_SOURCE "src/Lava*.cpp")

list(APPEND LAVA_SOURCE "extern/vkload/src/vkload.cpp")

set(DEMO_NAMES
    hellolava
    vtriangle
    vertexid)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} demos/${DEMO}.cpp ${LAVA_SOURCE})
endforeach()
