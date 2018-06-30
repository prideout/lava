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

# If submodules have been initialized, then build demo dependencies.
if(EXISTS "${CMAKE_SOURCE_DIR}/extras/glfw/CMakeLists.txt")
    add_subdirectory(extras/glfw)
    add_subdirectory(extras/glslang)
    add_subdirectory(extras/SimpleFileWatcher)
endif()

# Build the core library (zero dependencies).
include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(LAVA_SOURCE
    src/LavaContext.cpp
    src/LavaCpuBuffer.cpp
    src/LavaDescCache.cpp
    src/LavaGpuBuffer.cpp
    src/LavaInternal.cpp
    src/LavaLoader.cpp
    src/LavaLog.cpp
    src/LavaSurfCache.cpp
    src/LavaPipeCache.cpp
    src/LavaTexture.cpp)

if(AMBER_REQUIRED)
    set(AMBER_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AmberMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AmberCompiler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AmberProgram.cpp PARENT_SCOPE)
endif()

add_library(lava STATIC ${LAVA_SOURCE})

# Build demos if submodules have been initialized.
if(EXISTS "${CMAKE_SOURCE_DIR}/extras/glfw/CMakeLists.txt")
    add_subdirectory(demos)
endif()
