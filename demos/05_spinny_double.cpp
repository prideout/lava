// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaPipeCache.h>
#include <par/LavaDescCache.h>
#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

#include "vmath.h"

using namespace par;

static constexpr int DEMO_WIDTH = 512;
static constexpr int DEMO_HEIGHT = 512;
static constexpr float PI = 3.1415926535;

static const std::string vertShaderGLSL = AMBER_PREFIX_450 R"GLSL(
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 vert_color;
layout(binding = 0) uniform MatrixBlock {
  mat4 transform;
};
void main() {
    gl_Position = transform * vec4(position, 0, 1);
    vert_color = color;
})GLSL";

static const std::string fragShaderGLSL = AMBER_PREFIX_450 R"GLSL(
layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec4 vert_color;
void main() {
    frag_color = vert_color;
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
    GLFWwindow* window;
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "spinny", 0, 0);
        glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        });
    }

    // Create the VkInstance, VkDevice, etc.
    auto context = LavaContext::create({
        .depthBuffer = false, .validation = true,
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

    // Fill in a shared CPU-GPU vertex buffer.
    auto vertexBuffer = LavaCpuBuffer::create({
        .device = device, .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    });

    // Compile shaders.
    auto program = AmberProgram::create(vertShaderGLSL, fragShaderGLSL);
    program->compile(device);
    VkShaderModule vshader = program->getVertexShader();
    VkShaderModule fshader = program->getFragmentShader();

    // Create the double-buffered UBO.
    LavaCpuBuffer* ubo[2];
    for (int i = 0; i < 2; i++) {
        ubo[i] = LavaCpuBuffer::create({
            .device = device, .gpu = gpu, .size = sizeof(Matrix4),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        });
    }

    // Create the descriptor set.
    auto descriptors = LavaDescCache::create({
        .device = device,
        .uniformBuffers = { 0 },
        .imageSamplers = {}
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    auto pipelines = LavaPipeCache::create({
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
        .descriptorLayouts = { dlayout },
        .vshader = vshader,
        .fshader = fshader,
        .renderPass = renderPass
    });
    VkPipeline pipeline = pipelines->getPipeline();
    VkPipelineLayout playout = pipelines->getLayout();

    // Prepare for the draw loop.
    VkBuffer buffer[] = { vertexBuffer->getBuffer() };
    VkDeviceSize offsets[] = { 0 };
    VkViewport viewport = { .width = (float) extent.width, .height = (float) extent.height };
    VkRect2D scissor { .extent = extent };
    VkClearValue clearValue = { .color.float32 = {0.1, 0.2, 0.4, 1.0} };
    VkRenderPassBeginInfo rpbi {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .renderArea.extent = extent,
        .pClearValues = &clearValue,
        .clearValueCount = 1
    };

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the command buffer and begin the render pass.
        VkCommandBuffer cmdbuffer = context->beginFrame();
        rpbi.framebuffer = context->getFramebuffer();
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);

        // We know that ubo[1] is no longer being consumed by the GPU, so write over it and bind
        // a descriptor set that points to it.
        Matrix4 matrix = M4MakeRotationZ(glfwGetTime());
        ubo[1]->setData(&matrix, sizeof(matrix));
        std::swap(ubo[0], ubo[1]);
        descriptors->setUniformBuffer(0, ubo[0]->getBuffer());
        VkDescriptorSet dset = descriptors->getDescriptor();
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                &dset, 0, 0);

        // Make the draw call and end the render pass.
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();
    }

    // Wait for the command buffers to finish executing.
    context->waitFrame();

    // Cleanup.
    LavaDescCache::destroy(&descriptors);
    LavaCpuBuffer::destroy(&ubo[0]);
    LavaCpuBuffer::destroy(&ubo[1]);
    LavaCpuBuffer::destroy(&vertexBuffer);
    AmberProgram::destroy(&program);
    LavaPipeCache::destroy(&pipelines);
    LavaContext::destroy(&context);
    return 0;
}
