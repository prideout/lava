#include <par/LavaLoader.h>
#include <par/AmberApplication.h>

#define DEMO_NAME "shadertoy"

#ifdef __ANDROID__
    #include <android/log.h>
    #include <android_native_app_glue.h>
#else
    static constexpr int DEMO_WIDTH = 1794 / 2;
    static constexpr int DEMO_HEIGHT = 1080 / 2;
    #include <GLFW/glfw3.h>
#endif

#include <chrono>

using namespace par;

static AmberApplication* g_vulkanApp = nullptr;

static double get_current_time() {
    using namespace std;
    static auto start = chrono::high_resolution_clock::now();
    auto now = chrono::high_resolution_clock::now();
    return 0.001 * chrono::duration_cast<chrono::milliseconds>(now - start).count();
}

#ifdef __ANDROID__

static void handle_cmd(android_app* app, int32_t cmd) {
    auto createSurface = [app](VkInstance instance) {
        VkAndroidSurfaceCreateInfoKHR createInfo {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = app->window
        };
        VkSurfaceKHR surface;
        vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);
        return surface;
    };
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            g_vulkanApp = AmberApplication::createApp(DEMO_NAME, createSurface);
            break;
        case APP_CMD_TERM_WINDOW:
            delete g_vulkanApp;
            break;
        default:
            __android_log_print(ANDROID_LOG_INFO, "VulkanApp", "event not handled: %d", cmd);
    }
}

void android_main(struct android_app* app) {
    app->onAppCmd = handle_cmd;
    int events;
    android_poll_source* source;
    void** source_ptr = (void**) &source;
    do {
        auto result = ALooper_pollAll(g_vulkanApp ? 1 : 0, nullptr, &events, source_ptr);
        if (result >= 0 && source) {
              source->process(app, source);
        }
        if (g_vulkanApp) {
            g_vulkanApp->draw(get_current_time());
        }
    } while (app->destroyRequested == 0);
}

#else

int main(const int argc, const char *argv[]) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    auto window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, DEMO_NAME, 0, 0);

    auto createSurface = [window] (VkInstance instance) {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
        return surface;
    };

    g_vulkanApp = AmberApplication::createApp(DEMO_NAME, createSurface);

    glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
        if (action != GLFW_RELEASE) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        g_vulkanApp->handleKey(key);
    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        g_vulkanApp->draw(get_current_time());
    }
    delete g_vulkanApp;
    return 0;
}

#endif
