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
    AmberProgramImpl(const string& vshader, const string& fshader) noexcept;
    ~AmberProgramImpl() noexcept;
    VkShaderModule compileVertexShader(VkDevice device) noexcept;
    VkShaderModule compileFragmentShader(VkDevice device) noexcept;
    AmberCompiler* mCompiler;
    string mVertShader;
    string mFragShader;
    VkShaderModule mVertModule = VK_NULL_HANDLE;
    VkShaderModule mFragModule = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
};

LAVA_DEFINE_UPCAST(AmberProgram)

AmberProgram* AmberProgram::create(const string& vshader, const string& fshader) noexcept {
    return new AmberProgramImpl(vshader, fshader);
}

void AmberProgram::operator delete(void* ptr) noexcept {
    auto impl = (AmberProgramImpl*) ptr;
    ::delete impl;
}

AmberProgramImpl::~AmberProgramImpl() noexcept {
    if (mDevice) {
        vkDestroyShaderModule(mDevice, mVertModule, VKALLOC);
        vkDestroyShaderModule(mDevice, mFragModule, VKALLOC);
    }
    delete mCompiler;
}

AmberProgramImpl::AmberProgramImpl(const string& vshader, const string& fshader) noexcept :
        mCompiler(AmberCompiler::create()), mVertShader(vshader), mFragShader(fshader) {}

VkShaderModule AmberProgramImpl::compileVertexShader(VkDevice device) noexcept {
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

VkShaderModule AmberProgramImpl::compileFragmentShader(VkDevice device) noexcept {
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

bool AmberProgram::compile(VkDevice device) noexcept {
    AmberProgramImpl& impl = *upcast(this);
    impl.compileVertexShader(device);
    impl.compileFragmentShader(device);
    impl.mDevice = device;
    return impl.mVertModule && impl.mFragModule;
}

VkShaderModule AmberProgram::getVertexShader() const noexcept {
    return upcast(this)->mVertModule;
}

VkShaderModule AmberProgram::getFragmentShader() const noexcept {
    return upcast(this)->mFragModule;
}
