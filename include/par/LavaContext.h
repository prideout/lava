// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <par/LavaLoader.h>

namespace par {

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

    // General accessors.
    VkInstance getInstance() const noexcept;
    VkSurfaceKHR getSurface() const noexcept;
    VkExtent2D getSize() const noexcept;
    VkDevice getDevice() const noexcept;
    VkCommandPool getCommandPool() const noexcept;
    VkPhysicalDevice getGpu() const noexcept;
    const VkPhysicalDeviceFeatures& getGpuFeatures() const noexcept;
    VkQueue getQueue() const noexcept;
    VkFormat getFormat() const noexcept;
    VkColorSpaceKHR getColorSpace() const noexcept;
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const noexcept;
    VkRenderPass getRenderPass() const noexcept;
    VkSwapchainKHR getSwapchain() const noexcept;

    // Swap chain related accessors.
    VkCommandBuffer getCmdBuffer() const noexcept;
    VkImage getImage() const noexcept;
    VkImageView getImageView() const noexcept;
    VkFramebuffer getFramebuffer() const noexcept;

protected:
    LavaContext() noexcept = default;
    ~LavaContext() noexcept = default;
    LavaContext(LavaContext const&) = delete;
    LavaContext(LavaContext&&) = delete;
    LavaContext& operator=(LavaContext const&) = delete;
    LavaContext& operator=(LavaContext&&) = delete;
};

}
