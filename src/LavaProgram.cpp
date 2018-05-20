// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaCompiler.h>
#include <par/LavaLog.h>
#include <par/LavaProgram.h>

#include "LavaInternal.h"

using namespace par;
using namespace std;

class LavaProgramImpl : public LavaProgram {
public:
    LavaProgramImpl(string_view vshader, string_view fshader) noexcept;
    ~LavaProgramImpl() noexcept;
    VkShaderModule getVertexShader(VkDevice device) noexcept;
    VkShaderModule getFragmentShader(VkDevice device) noexcept;
    LavaCompiler* mCompiler;
    string_view mVertShader;
    string_view mFragShader;
    VkShaderModule mVertModule = VK_NULL_HANDLE;
    VkShaderModule mFragModule = VK_NULL_HANDLE;
    friend class LavaProgram;
};

LAVA_IMPL_CLASS(LavaProgram)

LavaProgram* LavaProgram::create(string_view vshader, string_view fshader) noexcept {
    return new LavaProgramImpl(vshader, fshader);
}

void LavaProgram::destroy(LavaProgram** that, VkDevice device) noexcept {
    LavaProgramImpl* impl = upcast(*that);
    vkDestroyShaderModule(device, impl->mVertModule, VKALLOC);
    vkDestroyShaderModule(device, impl->mFragModule, VKALLOC);
    delete upcast(impl);
    *that = nullptr;
}

LavaProgramImpl::LavaProgramImpl(string_view vshader, string_view fshader) noexcept :
        mCompiler(LavaCompiler::create()), mVertShader(vshader), mFragShader(fshader) {}

LavaProgramImpl::~LavaProgramImpl() noexcept {
    LavaCompiler::destroy(&mCompiler);
}

VkShaderModule LavaProgramImpl::getVertexShader(VkDevice device) noexcept {
    if (mVertModule) {
        return mVertModule;
    }
    std::vector<uint32_t> spirv;
    bool success = mCompiler->compile(LavaCompiler::VERTEX, mVertShader, &spirv);
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

VkShaderModule LavaProgramImpl::getFragmentShader(VkDevice device) noexcept {
    if (mFragModule) {
        return mFragModule;
    }
    std::vector<uint32_t> spirv;
    bool success = mCompiler->compile(LavaCompiler::FRAGMENT, mFragShader, &spirv);
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

VkShaderModule LavaProgram::getVertexShader(VkDevice device) noexcept {
    return upcast(this)->getVertexShader(device);
}

VkShaderModule LavaProgram::getFragmentShader(VkDevice device) noexcept {
    return upcast(this)->getFragmentShader(device);
}
