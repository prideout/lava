// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <string>
#include <vector>

namespace par {

class AmberCompiler {
public:
    static AmberCompiler* create() noexcept;
    static void operator delete(void* ptr) noexcept;
    enum Stage { VERTEX, FRAGMENT, COMPUTE };
    bool compile(Stage stage, const std::string& glsl, std::vector<uint32_t>* spirv) const noexcept;
protected:
    AmberCompiler() noexcept = default;
    // par::noncopyable
    AmberCompiler(AmberCompiler const&) = delete;
    AmberCompiler(AmberCompiler&&) = delete;
    AmberCompiler& operator=(AmberCompiler const&) = delete;
    AmberCompiler& operator=(AmberCompiler&&) = delete;
};

}
