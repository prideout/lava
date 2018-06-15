#pragma once

#include <par/LavaLoader.h>
#include <android_native_app_glue.h>

VkResult buildShaderFromFile(
    android_app* appInfo,
    const char* filePath,
    VkShaderStageFlagBits type,
    VkDevice vkDevice,
    VkShaderModule* shaderOut);
