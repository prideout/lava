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
    struct AttachmentConfig;

    // Construction / Destruction.
    static LavaSurfCache* create(const Config& config) noexcept;
    static void operator delete(void* );

    // Factory functions for VkImage / VkImageLayout.
    Attachment const* createColorAttachment(const AttachmentConfig& config) const noexcept;
    void finalizeAttachment(Attachment const* attachment, VkCommandBuffer cmdbuf) const noexcept;
    void finalizeAttachment(Attachment const* attachment, VkCommandBuffer cmdbuf,
            VkBuffer srcData, uint32_t nbytes) const noexcept;
    void freeAttachment(Attachment const* attachment) const noexcept;

    // Cache retrieval / creation / eviction.
    VkFramebuffer getFramebuffer(const Params& params) noexcept;
    VkRenderPass getRenderPass(const Params& params, VkRenderPassBeginInfo* = nullptr) noexcept;
    void releaseUnused(uint64_t milliseconds) noexcept;

    struct Config {
        VkDevice device;
        VkPhysicalDevice gpu;
    };

    struct AttachmentConfig {
        uint32_t width;
        uint32_t height;
        VkFormat format;
        bool enableUpload;
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
