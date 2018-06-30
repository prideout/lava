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

struct AttachmentImpl : LavaSurfCache::Attachment {
    VmaAllocation mem;
};

struct FbCacheKey {
    LavaSurfCache::Attachment* color;
    LavaSurfCache::Attachment* depth;
};

struct FbCacheVal {
    VkFramebuffer handle;
    uint64_t timestamp;
    FbCacheVal(FbCacheVal const&) = delete;
    FbCacheVal& operator=(FbCacheVal const&) = delete;
    FbCacheVal(FbCacheVal &&) = default;
    FbCacheVal& operator=(FbCacheVal &&) = default;
};

struct RpCacheVal {
    VkRenderPass handle;
    uint64_t timestamp;
    RpCacheVal(RpCacheVal const&) = delete;
    RpCacheVal& operator=(RpCacheVal const&) = delete;
    RpCacheVal(RpCacheVal &&) = default;
    RpCacheVal& operator=(RpCacheVal &&) = default;
};

using RpCacheKey = LavaSurfCache::Params;

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

LavaSurfCache* LavaSurfCache::create(Config config) noexcept {
    auto impl = new LavaSurfCacheImpl;
    impl->device = config.device;
    impl->vma = getVma(config.device, config.gpu);
    return impl;
}

void LavaSurfCache::operator delete(void* ptr) {
    auto impl = (LavaSurfCacheImpl*) ptr;
    ::delete impl;
}

LavaSurfCache::Attachment const* LavaSurfCache::createAttachment(
        uint32_t width, uint32_t height, VkFormat format) const noexcept {
    AttachmentImpl* impl = new AttachmentImpl();
    // impl->mem = ?;
    return impl;
}

void LavaSurfCache::finalizeAttachment(Attachment const* attachment,
        VkCommandBuffer cmdbuf) const noexcept {

}

void LavaSurfCache::freeAttachment(Attachment const* attachment) const noexcept {
    delete attachment;
}

VkFramebuffer LavaSurfCache::getFramebuffer(const Params& params) noexcept {
    return {};
}

VkRenderPass LavaSurfCache::getRenderPass(const Params& params) noexcept {
    return {};
}

void LavaSurfCache::releaseUnused(uint64_t milliseconds) noexcept {
    LavaSurfCacheImpl* impl = upcast(this);
    const uint64_t expiration = getCurrentTime() - milliseconds;

    auto& fbcache = impl->fbcache;
    using FbIter = decltype(impl->fbcache)::const_iterator;
    for (FbIter iter = fbcache.begin(); iter != fbcache.end();) {
        if (iter->second.timestamp < expiration) {
            LAVA_UNUSED VkFramebuffer fb = iter->second.handle;
            // TODO
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
            // TODO
            iter = rpcache.erase(iter);
        } else {
            ++iter;
        }
    }
}
