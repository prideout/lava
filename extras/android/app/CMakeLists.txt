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

add_subdirectory(lava)

add_library(native-lib SHARED src/main/cpp/native-lib.cpp)

find_library(log-lib log)
find_library(vulkan-lib vulkan)

target_link_libraries(native-lib ${log-lib} ${vulkan-lib} lava)
