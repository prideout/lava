// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaLog.h>

#define VMA_IMPLEMENTATION
#include "LavaInternal.h"

#include <chrono>
#include <unordered_map>

namespace par {

static std::unordered_map<VkDevice, VmaAllocator> sVmaAllocators;

VmaAllocator getVma(VkDevice device, VkPhysicalDevice gpu) {
    VmaAllocator& vma = sVmaAllocators[device];
    if (vma == VK_NULL_HANDLE) {
        createVma(device, gpu);
    }
    return vma;
}

void createVma(VkDevice device, VkPhysicalDevice gpu) {
    VmaVulkanFunctions funcs {
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
    };
    VmaAllocatorCreateInfo info = {
        .physicalDevice = gpu,
        .device = device,
        .pVulkanFunctions = &funcs,
    };
    auto& vma = sVmaAllocators[device] = VmaAllocator();
    vmaCreateAllocator(&info, &vma);
}

void destroyVma(VkDevice device) {
    vmaDestroyAllocator(sVmaAllocators[device]);
    sVmaAllocators[device] = VK_NULL_HANDLE;
}

uint64_t getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

size_t murmurHash(uint32_t const* words, uint32_t nwords, uint32_t seed) {
    uint32_t h = seed;
    size_t i = nwords;
    do {
        uint32_t k = *words++;
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
        h = (h << 13) | (h >> 19);
        h = (h * 5) + 0xe6546b64;
    } while (--i);
    h ^= nwords;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

}