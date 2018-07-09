// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>

#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaDescCache.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>
#include <par/LavaPipeCache.h>

#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

#include "vmath.h"

using namespace par;

namespace {
    constexpr int DEMO_WIDTH = 512;
    constexpr int DEMO_HEIGHT = 512;
    constexpr float PI = 3.1415926535;

    const std::string vertShaderGLSL = AMBER_PREFIX_450 R"GLSL(
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

    const std::string fragShaderGLSL = AMBER_PREFIX_450 R"GLSL(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec4 vert_color;
    void main() {
        frag_color = vert_color;
    })GLSL";

    struct Vertex {
        float position[2];
        uint32_t color;
    };

    const Vertex TRIANGLE_VERTICES[] {
        {{1, 0}, 0xffff0000u},
        {{cosf(PI * 2 / 3), sinf(PI * 2 / 3)}, 0xff00ff00u},
        {{cosf(PI * 4 / 3), sinf(PI * 4 / 3)}, 0xff0000ffu},
    };
}

static uint32_t selectMemoryType(uint32_t flags, VkFlags reqs, VkPhysicalDevice gpu) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if (flags & 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & reqs) == reqs) {
                return i;
            }
        }
        flags >>= 1;
    }
    return (uint32_t) ~0ul;
}

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
        .depthBuffer = false,
        .validation = false, // NOTE WELL: WHEN VALIDATION IS ENABLED, ISSUE GOES AWAY.
        .samples = VK_SAMPLE_COUNT_1_BIT,
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

    // Create the UBO and staging area.
    LavaCpuBuffer* uboStage = LavaCpuBuffer::create({
        .device = device, .gpu = gpu, .size = sizeof(Matrix4),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });

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
        .descriptorLayouts = { dlayout },
        .renderPass = renderPass,
        .vshader = vshader,
        .fshader = fshader,
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
        }
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
    VkBufferMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .size = VK_WHOLE_SIZE
    };
    const VkBufferCopy region = { .size = sizeof(Matrix4) };

    // Create GPU Buffer for testing purposes, without help from Lava.
    VkBufferCreateInfo bufinfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(Matrix4) * 32,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    VkBuffer gpubuf;
    VkDeviceMemory gpumem;
    VkMemoryRequirements memReqs;
    vkCreateBuffer(device, &bufinfo, nullptr, &gpubuf);
    vkGetBufferMemoryRequirements(device, gpubuf, &memReqs);
    VkMemoryAllocateInfo memAlloc {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = selectMemoryType(memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpu)
    };
    vkAllocateMemory(device, &memAlloc, nullptr, &gpumem);
    vkBindBufferMemory(device, gpubuf, gpumem, 0);

    // Main draw loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        rpbi.framebuffer = context->getFramebuffer();
        barrier.buffer = gpubuf;
        descriptors->setUniformBuffer(0, gpubuf);

        // Populate the CPU-side uniforms.
        Matrix4 matrix = M4MakeRotationZ(glfwGetTime());
        uboStage->setData(&matrix, sizeof(matrix));

        VkCommandBuffer cmdbuffer = context->beginFrame();

        // Copy the CPU buffer to the GPU buffer.
        // Note that this can be removed, obviously resulting in a blank screen, but the segfault
        // still occurs.
        if (true) {
            vkCmdCopyBuffer(cmdbuffer, uboStage->getBuffer(), gpubuf, 1, &region);
            vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        }

        // Issue commands for the render pass and draw call.
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                descriptors->getDescPointer(), 0, 0);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();

        // For testing purposes, destroy and recreate the GPU buffer used for uniforms.
        if (true) {
            vkDeviceWaitIdle(device);
            vkDestroyBuffer(device, gpubuf, nullptr);
            vkFreeMemory(device, gpumem, nullptr);
            vkCreateBuffer(device, &bufinfo, nullptr, &gpubuf);
            vkGetBufferMemoryRequirements(device, gpubuf, &memReqs);
            vkAllocateMemory(device, &memAlloc, nullptr, &gpumem);
            vkBindBufferMemory(device, gpubuf, gpumem, 0);
        }

        descriptors->evictDescriptors(0, 2);
    }

    vkDestroyBuffer(device, gpubuf, nullptr);
    vkFreeMemory(device, gpumem, nullptr);

    // Cleanup.
    delete descriptors;
    delete uboStage;
    delete vertexBuffer;
    delete pipelines;
    delete program;
    delete context;
    return 0;
}
