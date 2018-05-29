// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <string_view>
#include <vector>

namespace par {

class AmberCompiler {
public:
    static AmberCompiler* create() noexcept;
    static void destroy(AmberCompiler**) noexcept;
    enum Stage { VERTEX, FRAGMENT, COMPUTE };
    bool compile(Stage stage, std::string_view glsl, std::vector<uint32_t>* spirv) const noexcept;
protected:
    // par::heaponly
    AmberCompiler() noexcept = default;
    ~AmberCompiler() noexcept = default;
    // par::noncopyable
    AmberCompiler(AmberCompiler const&) = delete;
    AmberCompiler(AmberCompiler&&) = delete;
    AmberCompiler& operator=(AmberCompiler const&) = delete;
    AmberCompiler& operator=(AmberCompiler&&) = delete;
};

}
