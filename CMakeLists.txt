cmake_minimum_required(VERSION 3.6)

project(LavaDemos)

# To override the Debug build type, invoke cmake with -DCMAKE_BUILD_TYPE=Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# LLVM 5 or greater is required for -std=c++17
find_package(LLVM 5.0 REQUIRED CONFIG)

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
