/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "CreateShaderModule.h"
#include <android/log.h>
#include <par/LavaCompiler.h>

// Translate Vulkan Shader Type to shaderc shader type
shaderc_shader_kind getShadercShaderType(VkShaderStageFlagBits type) {
  switch (type) {
    case VK_SHADER_STAGE_VERTEX_BIT:
      return shaderc_glsl_vertex_shader;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
      return shaderc_glsl_fragment_shader;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
      return shaderc_glsl_tess_control_shader;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
      return shaderc_glsl_tess_evaluation_shader;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
      return shaderc_glsl_geometry_shader;
    case VK_SHADER_STAGE_COMPUTE_BIT:
      return shaderc_glsl_compute_shader;
    default:
      __android_log_assert("invalid VKShaderStageFlagBits",
                           "tutorial06_texture", "type = %08x", type);
  }
  return static_cast<shaderc_shader_kind>(-1);
}

// Create VK shader module from given glsl shader file
// filePath: glsl shader file (including path ) in APK's asset folder
VkResult buildShaderFromFile(android_app* appInfo, const char* filePath,
                             VkShaderStageFlagBits type, VkDevice vkDevice,
                             VkShaderModule* shaderOut) {
  // read file from Assets
  AAsset* file = AAssetManager_open(appInfo->activity->assetManager, filePath,
                                    AASSET_MODE_BUFFER);
  size_t glslShaderLen = AAsset_getLength(file);
  std::vector<char> glslShader;
  glslShader.resize(glslShaderLen);

  AAsset_read(file, static_cast<void*>(glslShader.data()), glslShaderLen);
  AAsset_close(file);

  // compile into spir-V shader
  shaderc_compiler_t compiler = shaderc_compiler_initialize();
  shaderc_compilation_result_t spvShader = shaderc_compile_into_spv(
      compiler, glslShader.data(), glslShaderLen, getShadercShaderType(type),
      "shaderc_error", "main", nullptr);
  if (shaderc_result_get_compilation_status(spvShader) !=
      shaderc_compilation_status_success) {
    return static_cast<VkResult>(-1);
  }

  // build vulkan shader module
  VkShaderModuleCreateInfo shaderModuleCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .codeSize = shaderc_result_get_length(spvShader),
      .pCode = (const uint32_t*)shaderc_result_get_bytes(spvShader),
      .flags = 0,
  };
  VkResult result = vkCreateShaderModule(vkDevice, &shaderModuleCreateInfo,
                                         nullptr, shaderOut);

  shaderc_result_release(spvShader);
  shaderc_compiler_release(compiler);

  return result;
}