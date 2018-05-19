// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaCompiler.h>
#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <GLFW/glfw3.h>

#define DEMO_WIDTH 640
#define DEMO_HEIGHT 480
#define VKALLOC nullptr

using namespace par;

static const std::string vertShaderGLSL = R"GLSL(
#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;
layout(location=0) out vec2 TexCoord;
void main() {
    gl_Position = position;
    TexCoord = uv;
}
)GLSL";

static const std::string fragShaderGLSL = R"GLSL(
#version 450
layout(location=0) out vec4 Color;
layout(location=0) in vec2 uv;
layout(binding=0, set=0) uniform sampler2D tex;
void main() {
    Color = texture(tex, uv);
}
)GLSL";

int main(const int argc, const char *argv[]) {
    // Initialize GLFW.
    glfwSetErrorCallback([] (int error, const char* description) {
        llog.error(description);
    });
    LOG_CHECK(glfwInit(), "Cannot initialize GLFW.");

    // Check if this is a high DPI display.
    float xscale = 0, yscale = 0;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);

    // Create the window.
    constexpr GLFWmonitor* monitor = nullptr;
    constexpr GLFWwindow* share = nullptr;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    // glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(DEMO_WIDTH / xscale, DEMO_HEIGHT / yscale, "noscene",
            monitor, share);
    if (!window) {
        llog.fatal("Cannot create a window in which to draw!");
    }

    // Allow the Escape key to quit.
    glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });

    // First create the VkInstance, then the platform-specific VkSurfaceKHR.
    static constexpr bool USE_VALIDATION = true;
    auto context = LavaContext::create(USE_VALIDATION);
    VkInstance instance = context->getInstance();
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    // Create the VkDevice and all related objects (command queue etc)
    context->initDevice(surface, true);
    VkDevice device = context->getDevice();

    // Compile shaders.
    auto compiler = LavaCompiler::create();
    std::vector<uint32_t> vertShaderSPIRV;
    bool success = compiler->compile(LavaCompiler::VERTEX, vertShaderGLSL, &vertShaderSPIRV);
    if (!success) {
        llog.fatal("vshader badness!");
    }
    std::vector<uint32_t> fragShaderSPIRV;
    success = compiler->compile(LavaCompiler::FRAGMENT, fragShaderGLSL, &fragShaderSPIRV);
    if (!success) {
        llog.fatal("fshader badness!");
    }
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fragShaderSPIRV.size() * 4,
        .pCode = fragShaderSPIRV.data()
    };
    VkResult err = vkCreateShaderModule(device, &moduleCreateInfo, VKALLOC, &module);
    LOG_CHECK(!err, "Unable to create shader module.");
    LavaCompiler::destroy(&compiler);

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // TODO: draw

        vkDeviceWaitIdle(device);
    }

    // Cleanup.
    vkDestroyShaderModule(device, module, VKALLOC);
    context->killDevice();
    LavaContext::destroy(&context);
    return 0;
}