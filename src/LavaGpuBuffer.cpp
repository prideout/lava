// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace par;

struct LavaGpuBufferImpl : LavaGpuBuffer {
    LavaGpuBufferImpl(Config config) noexcept;
    ~LavaGpuBufferImpl() noexcept;
    VkDevice device;
    VkBuffer buffer;
    VmaAllocation memory;
    VmaAllocator vma;
};

LAVA_DEFINE_UPCAST(LavaGpuBuffer)

LavaGpuBuffer* LavaGpuBuffer::create(Config config) noexcept {
    return new LavaGpuBufferImpl(config);
}

void LavaGpuBuffer::operator delete(void* ptr) {
    auto impl = (LavaGpuBufferImpl*) ptr;
    impl->~LavaGpuBufferImpl();
}

LavaGpuBufferImpl::~LavaGpuBufferImpl() noexcept {
    vmaDestroyBuffer(vma, buffer, memory);
}

LavaGpuBufferImpl::LavaGpuBufferImpl(Config config) noexcept : device(config.device) {
    assert(config.device && config.gpu && config.size > 0);
    vma = getVma(config.device, config.gpu);
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.size,
        .usage = config.usage
    };
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateBuffer(vma, &bufferInfo, &allocInfo, &buffer, &memory, nullptr);
}

VkBuffer LavaGpuBuffer::getBuffer() const noexcept {
    return upcast(this)->buffer;
}
