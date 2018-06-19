#pragma once

#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace par {

// Shields the Lava samples from the windowing system (GLFW on Desktop, native_app_glue
// on Android). The entry point (main or android_main) calls createApp() and draw().
struct AmberApplication {
    using SurfaceFn = std::function<VkSurfaceKHR(VkInstance)>;
    using FactoryFn = std::function<AmberApplication*(SurfaceFn)>;
    virtual ~AmberApplication() {}
    virtual void draw(double seconds) = 0;

    static std::unordered_map<std::string, FactoryFn>& registry() {
        static std::unordered_map<std::string, FactoryFn> r;
        return r;
    }

    static AmberApplication* createApp(std::string id, SurfaceFn createSurface) {
        return registry()[id](createSurface);
    }

    struct Register {
        Register(std::string id, FactoryFn createApp) {
            registry()[id] = createApp;
        }
    };
};

}