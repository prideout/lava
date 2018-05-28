// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace par;

struct LavaCpuBufferImpl : LavaCpuBuffer {
    LavaCpuBufferImpl(Config config) noexcept;
    VkDevice device;
    VkBuffer buffer;
    VmaAllocation memory;
    VmaAllocator vma;
};

LAVA_DEFINE_UPCAST(LavaCpuBuffer)

LavaCpuBuffer* LavaCpuBuffer::create(Config config) noexcept {
    return new LavaCpuBufferImpl(config);
}

void LavaCpuBuffer::destroy(LavaCpuBuffer** that) noexcept {
    LavaCpuBufferImpl* impl = upcast(*that);
    vmaDestroyBuffer(impl->vma, impl->buffer, impl->memory);
    delete upcast(impl);
    *that = nullptr;
}

LavaCpuBufferImpl::LavaCpuBufferImpl(Config config) noexcept : device(config.device) {
    assert(config.device && config.gpu && config.size > 0);
    vma = getVma(config.device, config.gpu);
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.size,
        .usage = config.usage
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vmaCreateBuffer(vma, &bufferInfo, &allocInfo, &buffer, &memory, nullptr);
    if (config.source) {
        setData(config.source, config.size);
    }
}

void LavaCpuBuffer::setData(void const* sourceData, uint32_t bytesToCopy) noexcept {
    auto impl = upcast(this);
    void* mappedData;
    vmaMapMemory(impl->vma, impl->memory, &mappedData);
    memcpy(mappedData, sourceData, bytesToCopy);
    vmaUnmapMemory(impl->vma, impl->memory);
}

VkBuffer LavaCpuBuffer::getBuffer() const noexcept {
    return upcast(this)->buffer;
}
