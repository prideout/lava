// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace std;
using namespace par;
using namespace spdlog;

class LavaContextImpl : public LavaContext {
public:
    LavaContextImpl(VkInstanceCreateInfo info) noexcept;
    ~LavaContextImpl() noexcept;
    void initialize(VkSurfaceKHR surface) noexcept;
    VkInstance getInstance() const noexcept {
        return mInstance;
    }
    VkDevice getDevice() const noexcept {
        return mDevice;
    }
    VkCommandBuffer getCmdBuffer() const noexcept {
        return mCmdBuffer;
    }
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkCommandBuffer mCmdBuffer = VK_NULL_HANDLE;
    LavaLog mLog;
};

LAVA_IMPL_CLASS(LavaContext)

LavaContext* LavaContext::create(VkInstanceCreateInfo info) noexcept {
    return new LavaContextImpl(info);
}

LavaContextImpl::LavaContextImpl(VkInstanceCreateInfo info) noexcept {
    VkResult result = vkCreateInstance(&info, VKALLOC, &mInstance);
    // LAVA_ASSERT(result == VK_SUCCESS, "Unable to create Vulkan instance.")
}

LavaContextImpl::~LavaContextImpl() noexcept {
    vkDestroyInstance(mInstance, VKALLOC);
}

void LavaContextImpl::initialize(VkSurfaceKHR surface) noexcept {
}

void LavaContext::initialize(VkSurfaceKHR surface) noexcept {
    upcast(this)->initialize(surface);
}

VkInstance LavaContext::getInstance() const noexcept {
    return upcast(this)->getInstance();
}

VkDevice LavaContext::getDevice() const noexcept {
    return upcast(this)->getDevice();
}

VkCommandBuffer LavaContext::getCmdBuffer() const noexcept {
    return upcast(this)->getCmdBuffer();
}
