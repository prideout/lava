cmake_minimum_required(VERSION 3.6)

project(LavaDemos)

file(GLOB_RECURSE AMBER_SOURCE "../src/Amber*.cpp")

set(DEMO_LIBS lava glfw glslang SPIRV)

if (APPLE)
    list(APPEND DEMO_LIBS "$ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.dylib")
endif()

link_libraries(${DEMO_LIBS})

include_directories(
    ../extras
    ../extras/glslang
    ../extras/par
    ../extras/stb
    ../extras/tinyobjloader
    ../include)

set(DEMO_NAMES
    01_clear_screen
    02_triangle_shared
    03_triangle_staged
    04_triangle_recorded
    05_spinny_double
    06_spinny_staged
    07_hello_texture
    08_klein_bottle)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} ${DEMO}.cpp ${AMBER_SOURCE})
    set_target_properties(${DEMO} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
