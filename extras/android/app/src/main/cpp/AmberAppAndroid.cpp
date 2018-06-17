#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR
#endif

#include <par/LavaLoader.h>
#include <vector>
#include <android_native_app_glue.h>
#include "AmberApp.h"

using namespace std;

#if defined(__GNUC__)
#  define LAVA_UNUSED __attribute__ ((unused))
#elif defined(_MSC_VER)
#  define LAVA_UNUSED __pragma(warning(suppress:4100))
#else
#  define LAVA_UNUSED
#endif

LAVA_UNUSED static const char* kTAG = "AmberApp";
#define LOGI(...)  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...)  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...)  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

namespace LavaLoader {
    bool init();
    void bind(VkInstance instance);
}

struct AmberAppImpl {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDevice device;
    bool ready = false;
};

AmberApp::AmberApp(void* platform_data) : self(*(new AmberAppImpl)) {

    android_app* app = (android_app*) platform_data;

    const vector<const char*> instance_extensions {
        "VK_KHR_surface"
        "VK_KHR_android_surface"
    };

    const vector<const char*> device_extensions { "VK_KHR_swapchain" };

    const VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_MAKE_VERSION(1, 0, 0),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName = "AmberApp",
        .pEngineName = "lava",
    };

    VkInstanceCreateInfo instanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
        .ppEnabledExtensionNames = instance_extensions.data(),
    };

    LavaLoader::init();

    vkCreateInstance(&instanceCreateInfo, nullptr, &self.instance);

    LavaLoader::bind(self.instance);

    VkAndroidSurfaceCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = app->window
    };

    vkCreateAndroidSurfaceKHR(self.instance, &createInfo, nullptr, &self.surface);

    self.ready = true;
}

AmberApp::~AmberApp() {
    // TODO: cleanup
    delete &self;
}

bool AmberApp::isReady() const {
    return self.ready;
}

void AmberApp::draw() {

}
