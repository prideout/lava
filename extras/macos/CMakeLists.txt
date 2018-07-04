cmake_minimum_required(VERSION 3.9.0)
project(lavamac)

set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++ -fno-rtti")

set(AMBER_REQUIRED True)
add_definitions(-DAMBER_COCOA)

add_subdirectory(lava)
add_subdirectory(lava/extras/glslang)

include_directories(
    lava/extras
    lava/extras/glslang
    lava/extras/par
    lava/extras/stb
    lava/extras/tinyobjloader
    lava/include)

add_executable(
    lavamac
    MACOSX_BUNDLE
    ${AMBER_SOURCE}
    src/AppDelegate.mm
    src/GameViewController.mm
    src/main.mm
    src/test.cpp
    src/AppDelegate.h
    src/GameViewController.h
    src/SharedStructures.h
    src/Shaders.metal
    src/MainMenu.xib)

set_source_files_properties(
    src/MainMenu.xib
    src/Shaders.metal
    PROPERTIES
    MACOSX_PACKAGE_LOCATION
    Resources)

set_target_properties(
    lavamac
    PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
    ${CMAKE_CURRENT_LIST_DIR}/src/plist.in)

target_link_libraries(lavamac
    "-framework AppKit"
    "-framework Metal"
    "-framework MetalKit"
    "-framework ModelIO"
    lava
    glslang
    SPIRV)

if($ENV{VULKAN_SDK})
    target_link_libraries(lavamac "$ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.dylib")
else()
    # TODO: build a library for CI purposes...
endif()
