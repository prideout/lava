// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaDescCache.h>

#include <unordered_map>

#include <assert.h>

#include "LavaInternal.h"

namespace par {

namespace {

struct CacheKey {
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDescriptorImageInfo> imageSamplers;
};

struct CacheVal {
    VkDescriptorSet handle;
    uint64_t timestamp;
};

struct IsEqual {
    bool operator()(const VkDescriptorImageInfo& a, const VkDescriptorImageInfo& b) const {
        return a.sampler == b.sampler && a.imageView == b.imageView &&
                a.imageLayout == b.imageLayout;
    }
    bool operator()(const CacheKey& a, const CacheKey& b) const {
        if (a.uniformBuffers.size() != b.uniformBuffers.size()) {
            return false;
        }
        for (size_t i = 0; i < a.uniformBuffers.size(); ++i) {
            if (a.uniformBuffers[i] != b.uniformBuffers[i]) {
                return false;
            }
        }
        if (a.imageSamplers.size() != b.imageSamplers.size()) {
            return false;
        }
        for (size_t i = 0; i < a.imageSamplers.size(); ++i) {
            if (!(*this)(a.imageSamplers[i], b.imageSamplers[i])) {
                return false;
            }
        }
        return true;
    }
};

using Cache = std::unordered_map<CacheKey, CacheVal, MurmurHashFn<CacheKey>, IsEqual>;

namespace DirtyFlag {
    static constexpr uint8_t UNIFORM_BUFFER = 1 << 0; 
    static constexpr uint8_t IMAGE_SAMPLER = 1 << 1;
}

struct LavaDescCacheImpl : LavaDescCache {
    CacheVal* currentDescriptor = nullptr;
    VkDevice device;
    Cache cache;
    CacheKey currentState;
    uint8_t dirtyFlags = 0xf;
    VkDescriptorSetLayout layout;
    uint32_t numUniformBuffers;
    uint32_t numImageSamplers;
};

LAVA_DEFINE_UPCAST(LavaDescCache)

} // anonymous namespace

LavaDescCache* LavaDescCache::create(Config config) noexcept {
    assert(config.device);
    auto impl = new LavaDescCacheImpl;
    impl->device = config.device;
    impl->currentState = {
        .uniformBuffers = config.uniformBuffers,
        .imageSamplers = config.imageSamplers,
    };
    impl->numUniformBuffers = (uint32_t) config.uniformBuffers.size();
    impl->numImageSamplers = (uint32_t) config.imageSamplers.size();
    uint32_t numImageSamplerBindings;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(impl->numUniformBuffers + impl->numImageSamplers);
    uint32_t binding = 0;
    for (auto u : config.uniformBuffers) {
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = binding++,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
        });
    }
    for (auto u : config.imageSamplers) {
        bindings.emplace_back(VkDescriptorSetLayoutBinding {
            .binding = binding++,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
        });
    }

    VkDescriptorSetLayoutCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (uint32_t) bindings.size(),
        .pBindings = bindings.data()
    };
    vkCreateDescriptorSetLayout(impl->device, &info, VKALLOC, &impl->layout);

    VkDescriptorPoolSize poolSizes[2] = {};
    VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .pPoolSizes = &poolSizes[0],
        .maxSets = MAX_NUM_DESCRIPTORS,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    };
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = poolInfo.maxSets * impl->numUniformBuffers;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = poolInfo.maxSets * impl->numImageSamplers;
    vkCreateDescriptorPool(mDevice, &poolInfo, VKALLOC, &impl->descriptorPool);

    return impl;
}

void LavaDescCache::destroy(LavaDescCache** that) noexcept {
    LavaDescCacheImpl* impl = upcast(*that);
    for (auto pair : impl->cache) {
        // vkFreeDescriptorSet(impl->device, pair.second.handle, VKALLOC);
    }
    // vkDestroyPool
    vkDestroyDescriptorSetLayout(impl->device, impl->layout, VKALLOC);
    delete upcast(impl);
    *that = nullptr;
}

void LavaDescCache::releaseUnused(uint64_t milliseconds) noexcept {
    LavaDescCacheImpl* impl = upcast(this);
    const uint64_t expiration = getCurrentTime() - milliseconds;
    Cache cache;
    cache.swap(impl->cache);
    for (auto pair : impl->cache) {
        if (pair.second.timestamp >= expiration) {
            impl->cache.emplace(pair);
        }
    }
}

} // par namespace