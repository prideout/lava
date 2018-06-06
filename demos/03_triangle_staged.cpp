// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaPipeCache.h>
#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

#include <math.h>

using namespace par;

static constexpr int DEMO_WIDTH = 512;
static constexpr int DEMO_HEIGHT = 512;
static constexpr float PI = 3.1415926535;

static const std::string vertShaderGLSL = AMBER_PREFIX_450 R"GLSL(
layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
layout(location=0) out vec4 vert_color;
void main() {
    gl_Position = vec4(position, 0, 1);
    vert_color = color;
})GLSL";

static const std::string fragShaderGLSL = AMBER_PREFIX_450 R"GLSL(
layout(location=0) out vec4 frag_color;
layout(location=0) in vec4 vert_color;
void main() {
    frag_color = vert_color;;
})GLSL";

struct Vertex {
    float position[2];
    uint32_t color;
};

static const Vertex TRIANGLE_VERTICES[] {
    {{1, 0}, 0xffff0000u},
    {{cosf(PI * 2 / 3), sinf(PI * 2 / 3)}, 0xff00ff00u},
    {{cosf(PI * 4 / 3), sinf(PI * 4 / 3)}, 0xff0000ffu},
};

int main(const int argc, const char *argv[]) {
    // Initialize GLFW and create the window.
    glfwInit();
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
    const VkDevice device = context->getDevice();
    const VkPhysicalDevice gpu = context->getGpu();
    const VkRenderPass renderPass = context->getRenderPass();
    const VkExtent2D extent = context->getSize();

    // Fill in a staging area for the vertex buffer.
    LavaCpuBuffer* stage = LavaCpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    LavaGpuBuffer* vertexBuffer = LavaGpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });

    // Copy the contents of the staging area.
    VkCommandBuffer cmdbuffer = context->beginWork();
    const VkBufferCopy region = { .size = sizeof(TRIANGLE_VERTICES) };
    vkCmdCopyBuffer(cmdbuffer, stage->getBuffer(), vertexBuffer->getBuffer(), 1, &region);
    context->endWork();

    // Compile shaders.
    auto program = AmberProgram::create(vertShaderGLSL, fragShaderGLSL);
    program->compile(device);
    VkShaderModule vshader = program->getVertexShader();
    VkShaderModule fshader = program->getFragmentShader();

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    LavaPipeCache* pipelines = LavaPipeCache::create({
        .device = device,
        .vertex = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .attributes = { {
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = 0u,
                .offset = 0u,
            }, {
                .binding = 0u,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .location = 1u,
                .offset = 8u,
            } },
            .buffers = { {
                .binding = 0u,
                .stride = 12,
            } }
        },
        .descriptorLayouts = {},
        .vshader = vshader,
        .fshader = fshader,
        .renderPass = renderPass
    });
    VkPipeline pipeline = pipelines->getPipeline();

    // Ensure the VBO is ready before we start drawing.
    context->waitWork();
    LavaCpuBuffer::destroy(&stage);

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the command buffer and begin the render pass.
        VkCommandBuffer cmdbuffer = context->beginFrame();
        const VkClearValue clearValue = { .color.float32 = {0.1, 0.2, 0.4, 1.0} };
        const VkRenderPassBeginInfo rpbi {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .framebuffer = context->getFramebuffer(),
            .renderPass = renderPass,
            .renderArea.extent = extent,
            .pClearValues = &clearValue,
            .clearValueCount = 1
        };
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { .width = (float) extent.width, .height = (float) extent.height };
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);

        VkRect2D scissor { .extent = extent };
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkBuffer buffer[] = { vertexBuffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);

        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);

        // End the render pass, flush the command buffer, and present the backbuffer.
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();
    }

    // Wait for the command buffers to finish executing.
    context->waitFrame();

    // Cleanup.
    LavaGpuBuffer::destroy(&vertexBuffer);
    AmberProgram::destroy(&program);
    LavaPipeCache::destroy(&pipelines);
    LavaContext::destroy(&context);
    return 0;
}
