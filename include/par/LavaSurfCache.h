// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vulkan/vulkan.h>

namespace par {

// Creates offscreen rendering surfaces, manages a cache of VkFramebuffer and VkRenderPass.
class LavaSurfCache {
public:
    struct Config;
    struct Attachment;
    struct Params;

    // Construction / Destruction.
    static LavaSurfCache* create(const Config& config) noexcept;
    static void operator delete(void* );

    // Factory functions for VkImage / VkImageLayout.
    Attachment const* createColorAttachment(uint32_t w, uint32_t h, VkFormat) const noexcept;
    void finalizeAttachment(Attachment const* attachment, VkCommandBuffer cmdbuf) const noexcept;
    void freeAttachment(Attachment const* attachment) const noexcept;

    // Cache retrieval / creation / eviction.
    VkFramebuffer getFramebuffer(const Params& params) noexcept;
    VkRenderPass getRenderPass(const Params& params, VkRenderPassBeginInfo* = nullptr) noexcept;
    void releaseUnused(uint64_t milliseconds) noexcept;

    struct Config {
        VkDevice device;
        VkPhysicalDevice gpu;
    };

    struct Attachment {
        VkImage image;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;
        VkFormat format;
    };

    struct Params {
        Attachment const* color;
        Attachment const* depth;
        VkClearValue clearValue;
        bool discardColor;
        bool discardDepth;
        float clearDepth;
    };

protected:
    LavaSurfCache() noexcept = default;
    LavaSurfCache(LavaSurfCache const&) = delete;
    LavaSurfCache& operator=(LavaSurfCache const&) = delete;
};

using LavaSurface = LavaSurfCache::Params;

}
