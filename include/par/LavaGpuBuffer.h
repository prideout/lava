// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vulkan/vulkan.h>

namespace par {

class LavaGpuBuffer {
public:
    struct Config {
        VkDevice device;
        VkPhysicalDevice gpu;
        uint32_t size;
        VkBufferUsageFlags usage;
    };    
    static LavaGpuBuffer* create(Config config) noexcept;
    static void operator delete(void* );
    VkBuffer getBuffer() const noexcept;
protected:
    LavaGpuBuffer() noexcept = default;
    // par::noncopyable
    LavaGpuBuffer(LavaGpuBuffer const&) = delete;
    LavaGpuBuffer& operator=(LavaGpuBuffer const&) = delete;
};

}
