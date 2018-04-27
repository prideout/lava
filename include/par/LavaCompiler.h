// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <string_view>
#include <vector>

namespace par {

class LavaCompiler {
public:
    static LavaCompiler* create() noexcept;
    static void destroy(LavaCompiler**) noexcept;
    enum Stage { VERTEX, FRAGMENT, COMPUTE };
    bool compile(Stage stage, std::string_view glsl, std::vector<uint32_t>* spirv) const noexcept;
protected:
    LavaCompiler() noexcept = default;
    ~LavaCompiler() noexcept = default;
    LavaCompiler(LavaCompiler const&) = delete;
    LavaCompiler(LavaCompiler&&) = delete;
    LavaCompiler& operator=(LavaCompiler const&) = delete;
    LavaCompiler& operator=(LavaCompiler&&) = delete;
};

}
