cmake_minimum_required(VERSION 3.6)

project(Lava)

# To override the Debug build type, invoke cmake with -DCMAKE_BUILD_TYPE=Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# Note that -fsanitize=undefined is also an interesting option.
# -fsanitize=leak is also interesting, but not yet supported on MacOS.
# set(EXTRA_SANITIZE_OPTIONS "-fsanitize=address")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EXTRA_SANITIZE_OPTIONS}")

set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fno-rtti")

include_directories(include)

file(GLOB_RECURSE LAVA_SOURCE "src/Lava*.cpp")

# Build the core library (zero dependencies).
add_library(lava SHARED ${LAVA_SOURCE})

# If submodules have been initialized, then build demos too.
if(EXISTS "${CMAKE_SOURCE_DIR}/extras/glfw/CMakeLists.txt")
    add_subdirectory(extras/glfw)
    add_subdirectory(extras/glslang)
    add_subdirectory(extras/SimpleFileWatcher)
    add_subdirectory(demos)
endif()
