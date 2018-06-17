#pragma once

#include <vulkan/vulkan.h>

#include <functional>

struct AmberAppImpl;

struct AmberApp {
    using SurfaceFn = std::function<VkSurfaceKHR(VkInstance)>;
    static AmberApp* create(int appIndex, SurfaceFn createSurface);
    virtual ~AmberApp() {}
    virtual void draw(double seconds) = 0;
};
