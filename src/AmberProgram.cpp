// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/AmberCompiler.h>
#include <par/LavaLog.h>
#include <par/AmberProgram.h>

#include <vector>
#include <fstream>
#include <regex>

#include "LavaInternal.h"

using namespace par;
using namespace std;

#ifndef __ANDROID__
#include <FileWatcher/FileWatcher.h>
using namespace FW;
#endif

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
    #ifndef __ANDROID__
    FileWatcher mFileWatcher;
    struct : FileWatchListener {
        FileListener callback;
        void handleFileAction(WatchID watchid, const String& dir, const String& filename,
                Action action) override {
            if (action == Actions::Modified) {
                if (callback) {
                    callback(filename);
                }
            }
        }
    } mFileListener;
    #endif
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
    if (mDevice && mVertModule) {
        vkDestroyShaderModule(mDevice, mVertModule, VKALLOC);
    }
    if (mDevice && mFragModule) {
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
    if (!mCompiler->compile(AmberCompiler::VERTEX, mVertShader, &spirv)) {
        llog.error("Unable to compile vertex shader.");
        return VK_NULL_HANDLE;
    }
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
    if (!mCompiler->compile(AmberCompiler::FRAGMENT, mFragShader, &spirv)) {
        llog.error("Unable to compile fragment shader.");
        return VK_NULL_HANDLE;
    }
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

string AmberProgram::getChunk(const string& filename, const string& chunkName) noexcept {
    const regex pattern(R"([\s]+)");
    sregex_token_iterator it(chunkName.begin(), chunkName.end(), pattern, -1);
    vector<string> chunkids(it, {});
    vector<string> tokens;

    string chunk = "#version 450\n";
    for (auto chunkid : chunkids) {
        chunk += "#line ";
        ifstream ifs(filename);
        bool extracting = false;
        int lineno = 1;
        for (string line; getline(ifs, line); ++lineno) {
            sregex_token_iterator it(line.begin(), line.end(), pattern, -1);
            vector<string> tokens(it, {});
            if (tokens.size() >= 2 && tokens[1] == chunkid) {
                chunk += to_string(lineno + 1) + "\n";
                extracting = true;
                continue;
            } else if (!extracting) {
                continue;
            }
            if (!line.compare(0, 2, "--")) {
                extracting = false;
            } else {
                chunk += line + "\n";
            }
        }
    }
    return chunk;
}

void AmberProgram::watchDirectory(const string& directory, FileListener onChange) noexcept {
    #ifndef __ANDROID__
    AmberProgramImpl& impl = *upcast(this);
    impl.mFileListener.callback = onChange;
    impl.mFileWatcher.addWatch(directory, &impl.mFileListener);
    #endif
}

void AmberProgram::checkDirectory() noexcept {
    #ifndef __ANDROID__
    AmberProgramImpl& impl = *upcast(this);
    impl.mFileWatcher.update();
    #endif
}
