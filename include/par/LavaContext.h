// The MIT License
// Copyright (c) 2018 Philip Rideout

#pragma once

#include <vulkan/vulkan.h>

namespace par {

// Encapsulates a single device and instance. Also manages a double-buffered swap chain and two
// corresponding command buffers.
class LavaContext {
public:
    // Constructing the LavaContext creates the VkInstance and nothing else. All other Vulkan
    // objects managed by this class are created during initialize().
    static LavaContext* create(VkInstanceCreateInfo) noexcept;

    // Frees the instance, the device, the swap chain, and the command buffers.
    static void destroy(LavaContext**) noexcept;

    // Given a platform-specific surface, creates the device, swap chain, and command buffers.
    void initialize(VkSurfaceKHR surface) noexcept;

    // Accessors to Vulkan objects owned by this class.
    VkInstance getInstance() const noexcept;
    VkDevice getDevice() const noexcept;
    VkCommandBuffer getCmdBuffer() const noexcept;

protected:
    LavaContext() noexcept = default;
    ~LavaContext() noexcept = default;
    LavaContext(LavaContext const&) = delete;
    LavaContext(LavaContext&&) = delete;
    LavaContext& operator=(LavaContext const&) = delete;
    LavaContext& operator=(LavaContext&&) = delete;
};

}
