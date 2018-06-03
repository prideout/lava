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

if(APPLE)
    set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/macOS/include")
else()
    set(VULKAN_INCLUDE_DIR "extras/glfw/deps")
endif()

include_directories(
    "${VULKAN_INCLUDE_DIR}"
    "include")

file(GLOB_RECURSE LAVA_SOURCE "src/Lava*.cpp")

# Build the core library (zero dependencies).
add_library(lava SHARED ${LAVA_SOURCE})

# If submodules have been initialized, then build demos too.
if(EXISTS "${CMAKE_SOURCE_DIR}/extras/glfw/CMakeLists.txt")
    add_subdirectory(extras/glfw)
    add_subdirectory(extras/glslang)
    add_subdirectory(demos)
endif()
