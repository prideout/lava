// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace par {

// Creates and stashes descriptor sets that conform to a specific layout.
// Also creates and stashes a set of VkSampler objects.
//
// Creates a single VkDescriptorSetLayout upon construction and stores it as immutable state.
// Accepts state changes via setUniformBuffer and setSamplerBinding.
// Creates or fetches a descriptor set when getDescriptorSet() is called.
// Creates or fetches a sampler when getSampler() is called.
// Can optionally free least-recently-used samplers and desc sets once per frame (releaseUnused).
//
class LavaDescCache {
public:
    struct Config {
        VkDevice device;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDescriptorImageInfo> imageSamplers;
    };
    static LavaDescCache* create(Config config) noexcept;
    static void destroy(LavaDescCache**) noexcept;

    // Fetches the descriptor set layout that was created at construction.
    VkDescriptorSetLayout getLayout() const noexcept;

    // Fetches or creates a VkDescriptorSet corresponding to the layout that was established
    // during construction. Immediately calls vkUpdateDescriptorSets if a new descriptor set was
    // constructed. The caller is responsible for calling vkCmdBindDescriptorSet.
    VkDescriptorSet getDescriptor() noexcept;

    // Similar to getDescriptor but returns a weak reference for convenience.
    VkDescriptorSet* getDescPointer() noexcept;

    // Fetches or creates a VkDescriptorSet corresponding to the layout that was established
    // during construction. Returns true if the client should call vkCmdBindDescriptorSet.
    // Sets "writes" to a non-empty vector if the client should call vkUpdateDescriptorSets.
    bool getDescriptorSet(VkDescriptorSet* descriptorSet,
            std::vector<VkWriteDescriptorSet>* writes) noexcept;

    void setUniformBuffer(uint32_t bindingIndex, VkBuffer uniformBuffer) noexcept;
    void setImageSampler(uint32_t bindingIndex, VkDescriptorImageInfo binding) noexcept;

    // Evicts descriptor sets and samplers that were last used more than N milliseconds ago.
    void releaseUnused(uint64_t milliseconds) noexcept;
protected:
    // par::heaponly
    LavaDescCache() noexcept = default;
    ~LavaDescCache() noexcept = default;
    // par::noncopyable
    LavaDescCache(LavaDescCache const&) = delete;
    LavaDescCache& operator=(LavaDescCache const&) = delete;
};

}
