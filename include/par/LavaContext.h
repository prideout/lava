// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <par/LavaLoader.h>

namespace par {

// The LavaContext owns the Vulkan instance, device, swap chain, and command buffers.
//
// Clients should create the platform-specific VkSurface after creating LavaContext, but before
// initializing the device. For example:
//
//     LavaContext* ctx = LavaContext::create(true);
//     VkSurface surface = glfwCreateWindowSurface(ctx->getInstance(), ...);
//     ctx->initDevice(surface, true);
//     ...main app body...
//     ctx->killDevice();
//     glfwDestroySurface(surface);
//     LavaContext::destroy(&ctx);
//
class LavaContext {
public:
    // Constructs the LavaContext and creates the VkInstance.
    static LavaContext* create(bool useValidation) noexcept;

    // Frees the instance. Be sure to call killDevice() first.
    static void destroy(LavaContext**) noexcept;

    // Creates the device, framebuffer, swap chain, and command buffers.
    // The passed-in platform surface determines the dimensions of the swap chain.
    void initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept;

    // Destroys the device, swap chain, and command buffers.
    void killDevice() noexcept;

    // Swaps the current command buffer / framebuffer.
    // TODO: consider renaming to "acquire"
    void swap() noexcept;

    // General accessors.
    VkInstance getInstance() const noexcept;
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
