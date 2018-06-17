// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaLog.h>
#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

using namespace par;

static constexpr int DEMO_WIDTH = 640;
static constexpr int DEMO_HEIGHT = 480;

static const std::string vertShaderGLSL = AMBER_PREFIX_450 R"GLSL(
layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;
layout(location=0) out vec2 TexCoord;
void main() {
    gl_Position = position;
    TexCoord = uv;
}
)GLSL";

static const std::string fragShaderGLSL = AMBER_PREFIX_450 R"GLSL(
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

    // Create the window.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "shadertest", 0, 0);
    if (!window) {
        llog.fatal("Cannot create a window in which to draw!");
    }

    // Allow the Escape key to quit.
    glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });

    // Create the VkInstance, VkDevice, etc.
    auto context = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .createSurface = [window] (VkInstance instance) {
            VkSurfaceKHR surface;
            glfwCreateWindowSurface(instance, window, nullptr, &surface);
            return surface;
        }
    });
    VkDevice device = context->getDevice();

    // Compile shaders.
    auto program = AmberProgram::create(vertShaderGLSL, fragShaderGLSL);
    program->compile(device);

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the command buffer and begin the render pass.
        VkCommandBuffer cmdbuffer = context->beginFrame();
        const float red = fmod(glfwGetTime(), 1.0);
        const VkClearValue clearValue = { .color.float32 = {red, 0, 0, 1} };
        const VkRenderPassBeginInfo rpbi {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .framebuffer = context->getFramebuffer(),
            .renderPass = context->getRenderPass(),
            .renderArea.extent.width = context->getSize().width,
            .renderArea.extent.height = context->getSize().height,
            .pClearValues = &clearValue,
            .clearValueCount = 1
        };
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        // ...do not draw any geometry...

        // End the render pass, flush the command buffer, and present the backbuffer.
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();
    }

    // Cleanup.
    delete program;
    delete context;
    return 0;
}
