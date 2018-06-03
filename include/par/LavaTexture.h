// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vulkan/vulkan.h>

namespace par {

class LavaTexture {
public:
    struct Config {
        VkDevice device;
        VkPhysicalDevice gpu;
        uint32_t size;
        void const* source;
        VkImageCreateInfo info;
    };
    struct Properties {
        VkImageMemoryBarrier* barrier1;
        VkBufferImageCopy* upload;
        VkImageMemoryBarrier* barrier2;        
        VkBuffer stage;
        VkImage image;
        VkImageView view;
    };
    static LavaTexture* create(Config config) noexcept;
    static void destroy(LavaTexture**) noexcept;
    void uploadStage(VkCommandBuffer cmd) const noexcept;
    void freeStage() noexcept;
    const Properties& getProperties() const noexcept;
protected:
    // par::heaponly
    LavaTexture() noexcept = default;
    ~LavaTexture() noexcept = default;
    // par::noncopyable
    LavaTexture(LavaTexture const&) = delete;
    LavaTexture& operator=(LavaTexture const&) = delete;
};

}
