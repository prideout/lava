// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <string>

namespace par {

class AmberProgram {
public:
    static AmberProgram* create(const std::string& vshader, const std::string& fshader) noexcept;
    static void destroy(AmberProgram**, VkDevice device) noexcept;
    VkShaderModule getVertexShader(VkDevice device) noexcept;
    VkShaderModule getFragmentShader(VkDevice device) noexcept;
protected:
    // par::heaponly
    AmberProgram() noexcept = default;
    ~AmberProgram() noexcept = default;
    // par::noncopyable
    AmberProgram(AmberProgram const&) = delete;
    AmberProgram(AmberProgram&&) = delete;
    AmberProgram& operator=(AmberProgram const&) = delete;
    AmberProgram& operator=(AmberProgram&&) = delete;
};

}
