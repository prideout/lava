// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaLog.h>

#define VMA_IMPLEMENTATION
#include "LavaInternal.h"

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

}