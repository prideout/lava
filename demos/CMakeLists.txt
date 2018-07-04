cmake_minimum_required(VERSION 3.6)

project(LavaDemos)

set(AMBER_SOURCE
    ../src/AmberCompiler.cpp
    ../src/AmberProgram.cpp)

set(DEMO_LIBS glfw glslang SPIRV curl sfw_lib lava)

if(APPLE)
    if($ENV{VULKAN_SDK})
        list(APPEND DEMO_LIBS "$ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.dylib")
    endif()
endif()

link_libraries(${DEMO_LIBS})

include_directories(
    ../extras
    ../extras/glslang
    ../extras/par
    ../extras/stb
    ../extras/tinyobjloader
    ../extras/SimpleFileWatcher/include
    ../include)

set(DEMO_NAMES
    01_clear_screen
    02_triangle_shared
    03_triangle_staged
    04_triangle_recorded
    05_spinny_double
    06_spinny_staged
    07_hello_texture
    08_klein_bottle
    09_particle_system
    0a_particle_system)

# Build the Desktop-only GLFW demos.

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} ${DEMO}.cpp ${AMBER_SOURCE})
    set_target_properties(${DEMO} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()

## Build Mobile-friendly Amber demos.

set(AMBER_DEMOS
    0b_shadertoy
    0c_framebuffer)

foreach(DEMO ${AMBER_DEMOS})
    add_executable(${DEMO} ${DEMO}.cpp ${AMBER_SOURCE} ../src/AmberMain.cpp)
    set_target_properties(${DEMO} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
