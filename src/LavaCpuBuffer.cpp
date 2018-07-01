// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace par;

struct LavaCpuBufferImpl : LavaCpuBuffer {
    LavaCpuBufferImpl(Config config) noexcept;
    ~LavaCpuBufferImpl() noexcept;
    VkDevice device;
    VkBuffer buffer;
    VmaAllocation memory;
    VmaAllocator vma;
    uint32_t size;
};

LAVA_DEFINE_UPCAST(LavaCpuBuffer)

LavaCpuBuffer* LavaCpuBuffer::create(Config config) noexcept {
    return new LavaCpuBufferImpl(config);
}

void LavaCpuBuffer::operator delete(void* ptr) {
    auto impl = (LavaCpuBufferImpl*) ptr;
    ::delete impl;
}

LavaCpuBufferImpl::~LavaCpuBufferImpl() noexcept {
    vmaDestroyBuffer(vma, buffer, memory);
}

LavaCpuBufferImpl::LavaCpuBufferImpl(Config config) noexcept : device(config.device) {
    assert(config.device && config.gpu && config.size > 0);
    vma = getVma(config.device, config.gpu);
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.capacity ? config.capacity : config.size,
        .usage = config.usage
    };
    size = config.size;
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vmaCreateBuffer(vma, &bufferInfo, &allocInfo, &buffer, &memory, nullptr);
    if (config.source) {
        setData(config.source, config.size);
    }
}

void LavaCpuBuffer::setData(void const* sourceData, uint32_t bytesToCopy, uint32_t offset)
        noexcept {
    auto impl = upcast(this);
    LOG_CHECK(offset + bytesToCopy <= impl->size, "Out of bounds upload.");
    uint8_t* mappedData;
    vmaMapMemory(impl->vma, impl->memory, (void**) &mappedData);
    memcpy(mappedData + offset, sourceData, bytesToCopy);
    vmaUnmapMemory(impl->vma, impl->memory);
}

VkBuffer LavaCpuBuffer::getBuffer() const noexcept {
    return upcast(this)->buffer;
}
