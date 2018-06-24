#pragma once

#include <par/LavaLog.h>

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
    using Registry = std::unordered_map<std::string, FactoryFn>;
    virtual ~AmberApplication() {}
    virtual void draw(double seconds) = 0;
    virtual void handleKey(int key) {}

    struct Prefs {
        std::string title = "amber";
        std::string first = "shadertoy";
        uint32_t width = 1794 / 2;
        uint32_t height = 1080 / 2;
        bool decorated = true;
    };

   static Prefs& prefs() {
       static Prefs p;
       return p;
   }

    static Registry& registry() {
        static Registry r;
        return r;
    }

    static Registry::iterator& current() {
        static Registry::iterator iter;
        return iter;
    }

    static AmberApplication* createApp(std::string id, SurfaceFn createSurface) {
        auto& iter = current();
        auto& reg = registry();
        iter = reg.find(id);
        assert(iter != reg.end());
        llog.warn("Starting {}...", iter->first);
        return iter->second(createSurface);
    }

    static AmberApplication* createNextApp(SurfaceFn createSurface) {
        auto& iter = current();
        auto& reg = registry();
        ++iter;
        if (iter == reg.end()) {
            iter = reg.begin();
        }
        llog.warn("Starting {}...", iter->first);
        return iter->second(createSurface);
    }

   static AmberApplication* createPreviousApp(SurfaceFn createSurface) {
        auto& iter = current();
        auto& reg = registry();
        ++iter;
        if (iter == reg.end()) {
            iter = reg.begin();
        }
        llog.warn("Starting {}...", iter->first);
        return iter->second(createSurface);
    }

    struct Register {
        Register(std::string id, FactoryFn createApp) { registry()[id] = createApp; }
        Register(Prefs p) { prefs() = p; }
    };
};

}