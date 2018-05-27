// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

namespace par {

class LavaCpuBuffer {
public:
    struct Config {
        VkDevice device;      // required
        VkPhysicalDevice gpu; // required
        uint32_t size;        // number of bytes
        void const* source;   // if non-null, triggers a memcpy during construction
        VkBufferUsageFlags usage;
    };    
    static LavaCpuBuffer* create(Config config) noexcept;
    static void destroy(LavaCpuBuffer**) noexcept;
    VkBuffer getBuffer() const noexcept;
    void setData(void const* sourceData, uint32_t bytesToCopy) noexcept;
protected:
    LavaCpuBuffer() noexcept = default;
    ~LavaCpuBuffer() noexcept = default;
    LavaCpuBuffer(LavaCpuBuffer const&) = delete;
    LavaCpuBuffer(LavaCpuBuffer&&) = delete;
    LavaCpuBuffer& operator=(LavaCpuBuffer const&) = delete;
    LavaCpuBuffer& operator=(LavaCpuBuffer&&) = delete;
};

}
