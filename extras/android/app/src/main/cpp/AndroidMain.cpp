#include <android/log.h>
#include <android_native_app_glue.h>
#include "AmberApp.h"

AmberApp* g_vulkanApp;

void handle_cmd(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            g_vulkanApp = new AmberApp(app);
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
        auto result = ALooper_pollAll(g_vulkanApp->isReady(), nullptr, &events, source_ptr);
        if (result >= 0 && source) {
              source->process(app, source);
        }
        if (g_vulkanApp->isReady()) {
            g_vulkanApp->draw();
        }
    } while (app->destroyRequested == 0);
}
