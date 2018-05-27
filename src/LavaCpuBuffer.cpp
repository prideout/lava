// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace par;

class LavaCpuBufferImpl : public LavaCpuBuffer {
public:
    LavaCpuBufferImpl(Config config) noexcept;
    VkDevice mDevice;
    VkBuffer mBuffer;
    VmaAllocation mMemory;
    VmaAllocator mAlloc;
    friend class LavaCpuBuffer;
};

LAVA_IMPL_CLASS(LavaCpuBuffer)

LavaCpuBuffer* LavaCpuBuffer::create(Config config) noexcept {
    return new LavaCpuBufferImpl(config);
}

void LavaCpuBuffer::destroy(LavaCpuBuffer** that) noexcept {
    LavaCpuBufferImpl* impl = upcast(*that);
    vmaDestroyBuffer(impl->mAlloc, impl->mBuffer, impl->mMemory);
    delete upcast(impl);
    *that = nullptr;
}

LavaCpuBufferImpl::LavaCpuBufferImpl(Config config) noexcept : mDevice(config.device) {
    assert(config.device && config.gpu && config.size > 0);
    mAlloc = getVma(config.device, config.gpu);
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.size,
        .usage = config.usage
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vmaCreateBuffer(mAlloc, &bufferInfo, &allocInfo, &mBuffer, &mMemory, nullptr);
    if (config.source) {
        setData(config.source, config.size);
    }
}

void LavaCpuBuffer::setData(void const* sourceData, uint32_t bytesToCopy) noexcept {
    auto impl = upcast(this);
    void* mappedData;
    vmaMapMemory(impl->mAlloc, impl->mMemory, &mappedData);
    memcpy(mappedData, sourceData, bytesToCopy);
    vmaUnmapMemory(impl->mAlloc, impl->mMemory);
}

VkBuffer LavaCpuBuffer::getBuffer() const noexcept {
    return upcast(this)->mBuffer;
}
