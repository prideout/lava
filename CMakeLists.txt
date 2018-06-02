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

set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fno-rtti")

add_subdirectory(extras/glfw)
add_subdirectory(extras/glslang)

set(LIBS glfw glslang SPIRV)

if (APPLE)
    list(APPEND LIBS $ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.dylib)
endif()

link_libraries(${LIBS})

include_directories(${DEMO} PRIVATE
    ${VULKAN_INCLUDE_DIR}
    extras
    extras/glslang
    extras/stb
    extras/par
    include)

file(GLOB_RECURSE LAVA_SOURCE
    "src/Amber*.cpp"
    "src/Lava*.cpp")

set(DEMO_NAMES
    01_clear_screen
    02_triangle_shared
    03_triangle_staged
    04_triangle_recorded
    05_spinny_double
    06_spinny_staged
    07_hello_texture)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} demos/${DEMO}.cpp ${LAVA_SOURCE})
endforeach()
