// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>
#include <par/LavaPipeCache.h>
#include <par/LavaTexture.h>
#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

#include "vmath.h"

using namespace par;
using namespace std;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STRINGIFY(x) #x
#define STRINGIFY_(x) STRINGIFY(x)
#define SHADER_PREFIX "#version 450\n#line " STRINGIFY_(__LINE__) "\n"

namespace {
    constexpr int DEMO_WIDTH = 512;
    constexpr int DEMO_HEIGHT = 512;
    constexpr float PI = 3.1415926535;

    constexpr char const* vertShaderGLSL = SHADER_PREFIX R"GLSL(
    layout(location=0) in vec2 position;
    layout(location=1) in vec4 color;
    layout(location=0) out vec4 vert_color;
    void main() {
        gl_Position = vec4(position, 0, 1);
        vert_color = color;
    })GLSL";

    constexpr char const* fragShaderGLSL = SHADER_PREFIX R"GLSL(
    layout(location=0) out vec4 frag_color;
    layout(location=0) in vec4 vert_color;
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

static LavaTexture* create_texture(char const* filename, VkDevice device, VkPhysicalDevice gpu) {
    uint32_t width, height;
    int ok = stbi_info(filename, (int*) &width, (int*) &height, 0);
    if (!ok) {
        llog.error("{}: {}.", filename, stbi_failure_reason());
        exit(1);
    }
    llog.info("Loading texture {:4}x{:4} {}", width, height, filename);
    uint8_t* texels = stbi_load(filename, (int*) &width, (int*) &height, 0, 4);
    auto texture = LavaTexture::create({
        .device = device, .gpu = gpu,
        .size = width * height * 4u,
        .source = texels,
        .width = width,
        .height = height,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
    });
    stbi_image_free(texels);
    return texture;
}

static void run_demo(LavaContext* context, GLFWwindow* window) {
    const VkDevice device = context->getDevice();
    const VkPhysicalDevice gpu = context->getGpu();
    const VkRenderPass renderPass = context->getRenderPass();
    const VkExtent2D extent = context->getSize();

    auto wrap_texture = [device, gpu](char const* filename) {
        auto deleter = [](LavaTexture* ptr){ LavaTexture::destroy(&ptr); };
        auto ptr = create_texture(filename, device, gpu);
        return unique_ptr<LavaTexture, decltype(deleter)>(ptr, deleter);
    };

    auto wrap_program = [device](char const* vshader, char const* fshader) {
        auto deleter = [device](AmberProgram* ptr){ AmberProgram::destroy(&ptr, device); };
        auto ptr = AmberProgram::create(vshader, fshader);
        return unique_ptr<AmberProgram, decltype(deleter)>(ptr, deleter);
    };

    auto wrap_pipecache = [](LavaPipeCache::Config config) {
        auto deleter = [](LavaPipeCache* ptr){ LavaPipeCache::destroy(&ptr); };
        auto ptr = LavaPipeCache::create(config);
        return unique_ptr<LavaPipeCache, decltype(deleter)>(ptr, deleter);
    };

    auto wrap_buffer = [](LavaGpuBuffer::Config config) {
        auto deleter = [](LavaGpuBuffer* ptr){ LavaGpuBuffer::destroy(&ptr); };
        auto ptr = LavaGpuBuffer::create(config);
        return unique_ptr<LavaGpuBuffer, decltype(deleter)>(ptr, deleter);
    };

    auto program = wrap_program(vertShaderGLSL, fragShaderGLSL);
    auto backdrop = wrap_texture("../extras/assets/abstract.jpg");
    auto occlusion = wrap_texture("../extras/assets/klein.png");
    auto rust = wrap_texture("../extras/assets/rust.png");
    VkCommandBuffer workbuf = context->beginWork();
    backdrop->uploadStage(workbuf);
    occlusion->uploadStage(workbuf);
    rust->uploadStage(workbuf);

    auto vertexBuffer = wrap_buffer({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    auto vboStage = LavaCpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    const VkBufferCopy region = { .size = sizeof(TRIANGLE_VERTICES) };
    vkCmdCopyBuffer(workbuf, vboStage->getBuffer(), vertexBuffer->getBuffer(), 1, &region);

    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    auto pipelines = wrap_pipecache({
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
        .vshader = program->getVertexShader(device),
        .fshader = program->getFragmentShader(device),
        .renderPass = renderPass
    });
    VkPipeline pipeline = pipelines->getPipeline();

    context->endWork();
    context->waitWork();
    backdrop->freeStage();
    occlusion->freeStage();
    rust->freeStage();
    LavaCpuBuffer::destroy(&vboStage);

    // Record two command buffers.
    LavaRecording* frame = context->createRecording();
    for (uint32_t i = 0; i < 2; i++) {
        const VkCommandBuffer cmdbuffer = context->beginRecording(frame, i);
        const VkClearValue clearValue = { .color.float32 = {0.1, 0.2, 0.4, 1.0} };
        const VkRenderPassBeginInfo rpbi {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .framebuffer = context->getFramebuffer(i),
            .renderPass = renderPass,
            .renderArea.extent = extent,
            .pClearValues = &clearValue,
            .clearValueCount = 1
        };
        const VkViewport viewport = {
            .width = (float) extent.width,
            .height = (float) extent.height
        };
        const VkRect2D scissor { .extent = extent };
        const VkBuffer buffer[] = { vertexBuffer->getBuffer() };
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        context->endRecording();
    }

    // Main game loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        context->presentRecording(frame);
    }

    // Wait for the command buffer to finish executing.
    context->waitRecording(frame);

    // Cleanup.
    context->freeRecording(frame);
}

int main(const int argc, const char *argv[]) {
    GLFWwindow* window;
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "klein", 0, 0);
        glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        });
    }
    LavaContext* context = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .createSurface = [window] (VkInstance instance) {
            VkSurfaceKHR surface;
            glfwCreateWindowSurface(instance, window, nullptr, &surface);
            return surface;
        }
    });
    run_demo(context, window);
    LavaContext::destroy(&context);
    return 0;
}
