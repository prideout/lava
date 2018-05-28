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
    LavaGpuBuffer() noexcept = default;
    ~LavaGpuBuffer() noexcept = default;
    LavaGpuBuffer(LavaGpuBuffer const&) = delete;
    LavaGpuBuffer(LavaGpuBuffer&&) = delete;
    LavaGpuBuffer& operator=(LavaGpuBuffer const&) = delete;
    LavaGpuBuffer& operator=(LavaGpuBuffer&&) = delete;
};

}
