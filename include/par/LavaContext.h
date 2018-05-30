// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <functional>

namespace par {

struct LavaRecording;

// The LavaContext owns the Vulkan instance, device, swap chain, and command buffers.
class LavaContext {
public:
    struct Config {
        bool depthBuffer;
        bool validation;
        std::function<VkSurfaceKHR(VkInstance)> createSurface;
    };
    static LavaContext* create(Config config) noexcept;
    static void destroy(LavaContext**) noexcept;

    // Starts a new command buffer and returns it.
    VkCommandBuffer beginFrame() noexcept;

    // Submits the command buffer and presents the most recently rendered image.
    void endFrame() noexcept;

    // Waits for the most recently submitted frame to finish.
    void waitFrame() noexcept;

    // Similar to beginFrame/endFrame/waitFrame for non-presentation work.
    VkCommandBuffer beginWork() noexcept;
    void endWork() noexcept;
    void waitWork() noexcept;

    // Allows commands to be recorded and played back later.
    LavaRecording* createRecording() noexcept;
    VkCommandBuffer beginRecording(LavaRecording*, uint32_t i) noexcept;
    void endRecording() noexcept;
    void presentRecording(LavaRecording*) noexcept;
    void freeRecording(LavaRecording*) noexcept;
    void waitRecording(LavaRecording*) noexcept;

    // General accessors.
    VkInstance getInstance() const noexcept;
    VkSurfaceKHR getSurface() const noexcept;
    VkExtent2D getSize() const noexcept;
    VkDevice getDevice() const noexcept;
    VkPhysicalDevice getGpu() const noexcept;
    const VkPhysicalDeviceFeatures& getGpuFeatures() const noexcept;
    VkQueue getQueue() const noexcept;
    VkFormat getFormat() const noexcept;
    VkColorSpaceKHR getColorSpace() const noexcept;
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const noexcept;
    VkRenderPass getRenderPass() const noexcept;
    VkSwapchainKHR getSwapchain() const noexcept;

    // Swap chain related accessors.
    VkImage getImage(uint32_t i = 0) const noexcept;
    VkImageView getImageView(uint32_t i = 0) const noexcept;
    VkFramebuffer getFramebuffer(uint32_t i = 0) const noexcept;

protected:
    // par::heaponly
    LavaContext() noexcept = default;
    ~LavaContext() noexcept = default;
    // par::noncopyable
    LavaContext(LavaContext const&) = delete;
    LavaContext& operator=(LavaContext const&) = delete;
};

}
