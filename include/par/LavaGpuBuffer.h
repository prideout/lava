// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

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
    static void destroy(LavaGpuBuffer**) noexcept;
    VkBuffer getBuffer() const noexcept;
protected:
    // par::heaponly
    LavaGpuBuffer() noexcept = default;
    ~LavaGpuBuffer() noexcept = default;
    // par::noncopyable
    LavaGpuBuffer(LavaGpuBuffer const&) = delete;
    LavaGpuBuffer& operator=(LavaGpuBuffer const&) = delete;
};

}
