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
        uint32_t width;
        uint32_t height;
        VkFormat format;
    };
    static LavaTexture* create(Config config) noexcept;
    static void destroy(LavaTexture**) noexcept;
    void uploadStage(VkCommandBuffer cmd) const noexcept;
    void freeStage() noexcept;
    VkImageView getImageView() const noexcept;
protected:
    // par::heaponly
    LavaTexture() noexcept = default;
    ~LavaTexture() noexcept = default;
    // par::noncopyable
    LavaTexture(LavaTexture const&) = delete;
    LavaTexture& operator=(LavaTexture const&) = delete;
};

}
