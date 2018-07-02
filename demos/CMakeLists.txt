cmake_minimum_required(VERSION 3.6)

project(LavaDemos)

set(AMBER_SOURCE
    ../src/AmberCompiler.cpp
    ../src/AmberProgram.cpp)

set(DEMO_LIBS glfw glslang SPIRV curl sfw_lib lava)

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
    ../extras/SimpleFileWatcher/include
    ../include)

set(DEMO_NAMES 00_destroy)

foreach(DEMO ${DEMO_NAMES})
    add_executable(${DEMO} ${DEMO}.cpp ${AMBER_SOURCE})
    set_target_properties(${DEMO} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
