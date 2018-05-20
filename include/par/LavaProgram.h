// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <par/LavaLoader.h>

#include <string_view>

namespace par {

class LavaProgram {
public:
    static LavaProgram* create(std::string_view vshader, std::string_view fshader) noexcept;
    static void destroy(LavaProgram**, VkDevice device) noexcept;
    VkShaderModule getVertexShader(VkDevice device) noexcept;
    VkShaderModule getFragmentShader(VkDevice device) noexcept;
protected:
    LavaProgram() noexcept = default;
    ~LavaProgram() noexcept = default;
    LavaProgram(LavaProgram const&) = delete;
    LavaProgram(LavaProgram&&) = delete;
    LavaProgram& operator=(LavaProgram const&) = delete;
    LavaProgram& operator=(LavaProgram&&) = delete;
};

}