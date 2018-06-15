// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <functional>
#include <string>

#include <vulkan/vulkan.h>

namespace par {

class AmberProgram {
public:
    using string = std::string;
    static AmberProgram* create(const string& vshader, const string& fshader) noexcept;
    static void operator delete(void* ptr) noexcept;
    bool compile(VkDevice device) noexcept;
    VkShaderModule getVertexShader() const noexcept;
    VkShaderModule getFragmentShader() const noexcept;

    // Extracts a range of text from a file, spanning from "-- chunkName" until the next "--", or
    // until the end of the file.
    static string getChunk(const string& filename, const string& chunkName) noexcept;

    // Monitors a file for changes. Useful for hot-loading.
    using FileListener = std::function<void(string)>;
    void watchDirectory(const string& folder, FileListener onChange) noexcept;
    void checkDirectory() noexcept;

protected:
    AmberProgram() noexcept = default;
    // par::noncopyable
    AmberProgram(AmberProgram const&) = delete;
    AmberProgram& operator=(AmberProgram const&) = delete;
};

#define AMBER_STRINGIFY(x) #x
#define AMBER_STRINGIFY_(x) AMBER_STRINGIFY(x)
#define AMBER_PREFIX_450 "#version 450\n#line " AMBER_STRINGIFY_(__LINE__) "\n"

}
