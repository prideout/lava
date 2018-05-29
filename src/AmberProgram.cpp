// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/AmberCompiler.h>
#include <par/LavaLog.h>
#include <par/AmberProgram.h>

#include "LavaInternal.h"

using namespace par;
using namespace std;

struct AmberProgramImpl : AmberProgram {
    AmberProgramImpl(string_view vshader, string_view fshader) noexcept;
    ~AmberProgramImpl() noexcept;
    VkShaderModule getVertexShader(VkDevice device) noexcept;
    VkShaderModule getFragmentShader(VkDevice device) noexcept;
    AmberCompiler* mCompiler;
    string_view mVertShader;
    string_view mFragShader;
    VkShaderModule mVertModule = VK_NULL_HANDLE;
    VkShaderModule mFragModule = VK_NULL_HANDLE;
};

LAVA_DEFINE_UPCAST(AmberProgram)

AmberProgram* AmberProgram::create(string_view vshader, string_view fshader) noexcept {
    return new AmberProgramImpl(vshader, fshader);
}

void AmberProgram::destroy(AmberProgram** that, VkDevice device) noexcept {
    AmberProgramImpl* impl = upcast(*that);
    vkDestroyShaderModule(device, impl->mVertModule, VKALLOC);
    vkDestroyShaderModule(device, impl->mFragModule, VKALLOC);
    delete upcast(impl);
    *that = nullptr;
}

AmberProgramImpl::AmberProgramImpl(string_view vshader, string_view fshader) noexcept :
        mCompiler(AmberCompiler::create()), mVertShader(vshader), mFragShader(fshader) {}

AmberProgramImpl::~AmberProgramImpl() noexcept {
    AmberCompiler::destroy(&mCompiler);
}

VkShaderModule AmberProgramImpl::getVertexShader(VkDevice device) noexcept {
    if (mVertModule) {
        return mVertModule;
    }
    std::vector<uint32_t> spirv;
    bool success = mCompiler->compile(AmberCompiler::VERTEX, mVertShader, &spirv);
    LOG_CHECK(success, "Unable to compile vertex shader.");
    VkShaderModuleCreateInfo moduleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * 4,
        .pCode = spirv.data()
    };
    VkResult err = vkCreateShaderModule(device, &moduleCreateInfo, VKALLOC, &mVertModule);
    LOG_CHECK(!err, "Unable to create vertex shader module.");
    return mVertModule;
}

VkShaderModule AmberProgramImpl::getFragmentShader(VkDevice device) noexcept {
    if (mFragModule) {
        return mFragModule;
    }
    std::vector<uint32_t> spirv;
    bool success = mCompiler->compile(AmberCompiler::FRAGMENT, mFragShader, &spirv);
    LOG_CHECK(success, "Unable to compile fragment shader.");
    VkShaderModuleCreateInfo moduleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * 4,
        .pCode = spirv.data()
    };
    VkResult err = vkCreateShaderModule(device, &moduleCreateInfo, VKALLOC, &mFragModule);
    LOG_CHECK(!err, "Unable to create fragment shader module.");
    return mFragModule;
}

VkShaderModule AmberProgram::getVertexShader(VkDevice device) noexcept {
    return upcast(this)->getVertexShader(device);
}

VkShaderModule AmberProgram::getFragmentShader(VkDevice device) noexcept {
    return upcast(this)->getFragmentShader(device);
}
