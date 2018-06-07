// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace par {

class AmberProgram {
public:
    static AmberProgram* create(const std::string& vshader, const std::string& fshader) noexcept;
    ~AmberProgram() noexcept;
    bool compile(VkDevice device) noexcept;
    VkShaderModule getVertexShader() const noexcept;
    VkShaderModule getFragmentShader() const noexcept;
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
