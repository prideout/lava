// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaSurfCache.h>
#include <par/LavaLog.h>

#include <unordered_map>

#include "LavaInternal.h"

using namespace par;
using namespace std;

namespace {

enum AttachmentType {
    COLOR,
    DEPTH,
};

struct AttachmentImpl : LavaSurfCache::Attachment {
    VmaAllocation mem;
    AttachmentType type;
};

struct FbCacheKey {
    LavaSurfCache::Attachment const* color;
    LavaSurfCache::Attachment const* depth;
};

struct FbCacheVal {
    VkFramebuffer handle;
    uint64_t timestamp;
    FbCacheVal(FbCacheVal const&) = delete;
    FbCacheVal& operator=(FbCacheVal const&) = delete;
    FbCacheVal(FbCacheVal &&) = default;
    FbCacheVal& operator=(FbCacheVal &&) = default;
};

struct RpCacheKey {
    VkFormat clayout;
    VkFormat dlayout;
    bool discardColor;
    bool discardDepth;
    float clearDepth;
};

struct RpCacheVal {
    VkRenderPass handle;
    uint64_t timestamp;
    RpCacheVal(RpCacheVal const&) = delete;
    RpCacheVal& operator=(RpCacheVal const&) = delete;
    RpCacheVal(RpCacheVal &&) = default;
    RpCacheVal& operator=(RpCacheVal &&) = default;
};

struct FbIsEqual { bool operator()(const FbCacheKey& a, const FbCacheKey& b) const; };
struct FbHashFn { uint64_t operator()(const FbCacheKey& key) const; };
struct RpIsEqual { bool operator()(const RpCacheKey& a, const RpCacheKey& b) const; };
struct RpHashFn { uint64_t operator()(const RpCacheKey& key) const; };

using FbCache = unordered_map<FbCacheKey, FbCacheVal, FbHashFn, FbIsEqual>;
using RpCache = unordered_map<RpCacheKey, RpCacheVal, RpHashFn, RpIsEqual>;

struct LavaSurfCacheImpl : LavaSurfCache {
    VkDevice device;
    VmaAllocator vma;
    FbCache fbcache;
    RpCache rpcache;
};

LAVA_DEFINE_UPCAST(LavaSurfCache)

} // anonymous namespace

LavaSurfCache* LavaSurfCache::create(const Config& config) noexcept {
    auto impl = new LavaSurfCacheImpl;
    impl->device = config.device;
    impl->vma = getVma(config.device, config.gpu);
    return impl;
}

void LavaSurfCache::operator delete(void* ptr) {
    auto impl = (LavaSurfCacheImpl*) ptr;
    VkDevice device = impl->device;
    for (auto& pair : impl->fbcache) {
        vkDestroyFramebuffer(device, pair.second.handle, VKALLOC);
    }
    for (auto& pair : impl->rpcache) {
        vkDestroyRenderPass(device, pair.second.handle, VKALLOC);
    }
    ::delete impl;
}

LavaSurfCache::Attachment const* LavaSurfCache::createColorAttachment(
        uint32_t width, uint32_t height, VkFormat format) const noexcept {
    auto impl = upcast(this);
    AttachmentImpl* attach = new AttachmentImpl();
    attach->width = width;
    attach->height = height;
    attach->format = format;
    attach->type = COLOR;
    VkImageCreateInfo imageInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = {width, height, 1},
        .format = format,
        .mipLevels = 1,
        .arrayLayers = 1,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(impl->vma, &imageInfo, &allocInfo, &attach->image, &attach->mem, nullptr);
    VkImageViewCreateInfo colorViewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = attach->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vkCreateImageView(impl->device, &colorViewInfo, VKALLOC, &attach->imageView);
    return attach;
}

void LavaSurfCache::finalizeAttachment(Attachment const* attachment,
        VkCommandBuffer cmdbuf) const noexcept {
    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = attachment->image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };
    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void LavaSurfCache::freeAttachment(Attachment const* attachment) const noexcept {
    auto impl = upcast(this);
    auto attach = (AttachmentImpl const*) attachment;
    vmaDestroyImage(impl->vma, attach->image, attach->mem);
    vkDestroyImageView(impl->device, attach->imageView, VKALLOC);
    delete attach;
}

VkFramebuffer LavaSurfCache::getFramebuffer(const Params& params) noexcept {
    assert(params.color && !params.depth && "Not yet implemented.");
    const uint32_t width = params.color ? params.color->width : params.depth->width;
    const uint32_t height = params.color ? params.color->height : params.depth->height;
    auto impl = upcast(this);
    const FbCacheKey key {
        .color = params.color,
        .depth = params.depth,
    };
    auto iter = impl->fbcache.find(key);
    if (iter != impl->fbcache.end()) {
        FbCacheVal* val = (FbCacheVal*) &(iter->second);
        val->timestamp = getCurrentTime();
        return val->handle;
    }
    VkImageView attachments[2];
    uint32_t nattachments = 0;
    if (params.color) {
        attachments[nattachments++] = params.color->imageView;
    }
    if (params.depth) {
        attachments[nattachments++] = params.depth->imageView;
    }
    VkFramebufferCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = getRenderPass(params, nullptr),
        .width = width,
        .height = height,
        .layers = 1,
        .attachmentCount = nattachments,
        .pAttachments = attachments
    };
    VkFramebuffer framebuffer;
    vkCreateFramebuffer(impl->device, &info, VKALLOC, &framebuffer);
    impl->fbcache.emplace(make_pair(key, FbCacheVal {framebuffer, getCurrentTime()}));
    return framebuffer;
}

VkRenderPass LavaSurfCache::getRenderPass(const Params& params, VkRenderPassBeginInfo* rpbi)
        noexcept {
    assert(params.color && !params.depth && "Not yet implemented.");
    const uint32_t width = params.color ? params.color->width : params.depth->width;
    const uint32_t height = params.color ? params.color->height : params.depth->height;
    auto impl = upcast(this);
    const RpCacheKey key {
        .clayout = params.color ? params.color->format : VK_FORMAT_UNDEFINED,
        .dlayout = params.depth ? params.depth->format : VK_FORMAT_UNDEFINED,
        .discardColor = params.discardColor,
        .discardDepth = params.discardDepth,
        .clearDepth = params.clearDepth,
    };
    auto iter = impl->rpcache.find(key);
    VkRenderPassBeginInfo info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderArea.extent = {width, height},
        .pClearValues = &params.clearValue,
        .clearValueCount = 1
    };
    if (iter != impl->rpcache.end()) {
        RpCacheVal* val = (RpCacheVal*) &(iter->second);
        val->timestamp = getCurrentTime();
        if (rpbi) {
            info.framebuffer = getFramebuffer(params);
            info.renderPass = val->handle;
            *rpbi = info;
        }
        return val->handle;

    }
    const bool discard = params.discardColor;
    LavaVector<VkAttachmentDescription> rpattachments;
    rpattachments.push_back(VkAttachmentDescription {
         .format = params.color->format,
         .samples = VK_SAMPLE_COUNT_1_BIT,
         .loadOp = discard ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR,
         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
         .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
         .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
         .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    });
    const VkAttachmentReference colorref {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const LAVA_UNUSED VkAttachmentReference depthref {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorref,
    };
    const VkRenderPassCreateInfo rpinfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = rpattachments.size,
        .pAttachments = rpattachments.data,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    VkRenderPass renderPass;
    vkCreateRenderPass(impl->device, &rpinfo, VKALLOC, &renderPass);
    impl->rpcache.emplace(make_pair(key, RpCacheVal {renderPass, getCurrentTime()}));
    if (rpbi) {
        info.framebuffer = getFramebuffer(params);
        info.renderPass = renderPass;
        *rpbi = info;
    }
    return renderPass;
}

void LavaSurfCache::releaseUnused(uint64_t milliseconds) noexcept {
    LavaSurfCacheImpl* impl = upcast(this);
    const uint64_t expiration = getCurrentTime() - milliseconds;
    auto& fbcache = impl->fbcache;
    using FbIter = decltype(impl->fbcache)::const_iterator;
    for (FbIter iter = fbcache.begin(); iter != fbcache.end();) {
        if (iter->second.timestamp < expiration) {
            VkFramebuffer fb = iter->second.handle;
            vkDestroyFramebuffer(impl->device, fb, VKALLOC);
            iter = fbcache.erase(iter);
        } else {
            ++iter;
        }
    }
    auto& rpcache = impl->rpcache;
    using RpIter = decltype(impl->rpcache)::const_iterator;
    for (RpIter iter = rpcache.begin(); iter != rpcache.end();) {
        if (iter->second.timestamp < expiration) {
            LAVA_UNUSED VkRenderPass rp = iter->second.handle;
            vkDestroyRenderPass(impl->device, rp, VKALLOC);
            iter = rpcache.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool FbIsEqual::operator()(const FbCacheKey& a, const FbCacheKey& b) const {
    return a.color == b.color && a.depth == b.depth;
}

uint64_t FbHashFn::operator()(const FbCacheKey& key) const {
    uint64_t a = (uint64_t) key.color;
    uint64_t b = (uint64_t) key.depth;
    return a ^ b;
}

bool RpIsEqual::operator()(const RpCacheKey& a, const RpCacheKey& b) const {
    return a.clayout == b.clayout && a.dlayout == b.dlayout &&
            a.discardColor != b.discardColor && a.discardDepth != b.discardDepth &&
            a.clearDepth != b.clearDepth;
}

uint64_t RpHashFn::operator()(const RpCacheKey& key) const {
    size_t sz = sizeof(key);
    assert(0 == (sz & 3) && "Hashing requires a size that is a multiple of 4.");
    return murmurHash((uint32_t*) &key, sz / 4, 0u);
}
