// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaProgram.h>
#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <GLFW/glfw3.h>

#include <math.h>

using namespace par;

static constexpr int DEMO_WIDTH = 640;
static constexpr int DEMO_HEIGHT = 480;

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

struct Vertex {
    float position[2];
    uint32_t color;
};

static const Vertex TRIANGLE_VERTICES[] {
    {{1, 0}, 0xffff0000u},
    {{cosf(M_PI * 2 / 3), sinf(M_PI * 2 / 3)}, 0xff00ff00u},
    {{cosf(M_PI * 4 / 3), sinf(M_PI * 4 / 3)}, 0xff0000ffu},
};

int main(const int argc, const char *argv[]) {
    // Initialize GLFW.
    glfwInit();
    float xscale, yscale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    auto* window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "triangle", 0, 0);
    glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });

    // Create the VkInstance, VkDevice, etc.
    LavaContext* context = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .createSurface = [window] (VkInstance instance) {
            VkSurfaceKHR surface;
            glfwCreateWindowSurface(instance, window, nullptr, &surface);
            return surface;
        }
    });
    VkDevice device = context->getDevice();

    // Fill in a shared CPU-GPU vertex buffer.
    LavaCpuBuffer* vertexBuffer = LavaCpuBuffer::create({
        .device = device,
        .size = sizeof(TRIANGLE_VERTICES),
        .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    });

    // Compile shaders.
    auto program = LavaProgram::create(vertShaderGLSL, fragShaderGLSL);
    program->getVertexShader(device);
    program->getFragmentShader(device);

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the command buffer and begin the render pass.
        VkCommandBuffer cmdbuffer = context->beginFrame();
        const VkClearValue clearValue = { .color.float32 = {0.1, 0.2, 0.4, 1.0} };
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

        // Draw the triangle.
        vkCmdBindVertexBuffers(...);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);

        // End the render pass, flush the command buffer, and present the backbuffer.
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();
    }

    // Cleanup.
    LavaCpuBuffer::destroy(&vertexBuffer, device);
    LavaProgram::destroy(&program, device);
    LavaContext::destroy(&context);
    return 0;
}
