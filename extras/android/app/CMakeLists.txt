cmake_minimum_required(VERSION 3.4.1)

if (${ANDROID_ABI} STREQUAL "x86_64")
    include_directories(${ANDROID_SYSROOT}/usr/include/x86_64-linux-android)
elseif (${ANDROID_ABI} STREQUAL "x86")
    include_directories(${ANDROID_SYSROOT}/usr/include/i686-linux-android)
elseif (${ANDROID_ABI} STREQUAL "arm64-v8a")
    include_directories(${ANDROID_SYSROOT}/usr/include/aarch64-linux-android)
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    include_directories(${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi)
endif()

set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${APP_GLUE_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR")
add_library(appglue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)

add_subdirectory(lava)

set(SRC_DIR src/main/cpp)
include_directories(lava/include)

add_library(lavademos SHARED
        ${SRC_DIR}/AmberAppAndroid.cpp
        #${SRC_DIR}/VulkanMain.cpp
        #${SRC_DIR}/CreateShaderModule.cpp
        ${SRC_DIR}/AndroidMain.cpp)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
target_link_libraries(lavademos log android appglue lava)
