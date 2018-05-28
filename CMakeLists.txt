cmake_minimum_required(VERSION 3.6)

project(Lava)

# To override the Debug build type, invoke cmake with -DCMAKE_BUILD_TYPE=Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# We turn on the sanitizer for subprojects to prevent false positives.
# Note that -fsanitize=undefined is also an interesting option.
# set(EXTRA_SANITIZE_OPTIONS "-fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_SANITIZE_OPTIONS}")

add_subdirectory(extern/glfw)
add_subdirectory(extern/glslang)

link_libraries(
    $ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.dylib
    glfw
    glslang
    SPIRV)

include_directories(${DEMO} PRIVATE
    ${VULKAN_INCLUDE_DIR}
    extern/spdlog/include
    extern/glslang
    extern/VulkanMemoryAllocator/src
    include)

set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fno-rtti")

file(GLOB_RECURSE LAVA_SOURCE "src/Lava*.cpp")

set(DEMO_NAMES
    triangle_shared
    triangle_staged
    shader_test)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} demos/${DEMO}.cpp ${LAVA_SOURCE})
endforeach()
