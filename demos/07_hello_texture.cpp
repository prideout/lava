// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>

#include <par/AmberProgram.h>
#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaDescCache.h>
#include <par/LavaLog.h>
#include <par/LavaPipeCache.h>
#include <par/LavaTexture.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include <stb_image.h>
#pragma clang diagnostic pop

#include <GLFW/glfw3.h>

#include "vmath.h"

using namespace par;

namespace {
    constexpr int DEMO_WIDTH = 512;
    constexpr int DEMO_HEIGHT = 512;
    constexpr char const* TEXTURE_FILENAME = "../extras/assets/abstract.jpg";

    constexpr char const* VSHADER_GLSL = AMBER_PREFIX_450 R"(
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 uv;
    layout(location = 0) out vec2 vert_uv;
    void main() {
        gl_Position = vec4(position, 0, 1);
        vert_uv = uv;
    })";

    constexpr char const* FSHADER_GLSL = AMBER_PREFIX_450 R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec2 vert_uv;
    layout(binding = 0) uniform sampler2D img;
    void main() {
        frag_color = texture(img, vert_uv);
    })";

    struct Vertex {
        float position[2];
        float uv[2];
    };

    #define P +1
    #define N -1
    const Vertex VERTICES[] {
        {{P, P}, {1,1}},
        {{N, P}, {0,1}},
        {{P, N}, {1,0}},
        {{N, N}, {0,0}},
    };
    #undef N
    #undef P
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
        window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "texture", 0, 0);
        glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        });
    }

    // Create the VkInstance, VkDevice, etc.
    auto context = LavaContext::create({
        .depthBuffer = false, .validation = true, .samples = VK_SAMPLE_COUNT_1_BIT,
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
        .size = sizeof(VERTICES),
        .source = VERTICES,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    });

    // Compile shaders.
    auto program = AmberProgram::create(VSHADER_GLSL, FSHADER_GLSL);
    program->compile(device);
    VkShaderModule vshader = program->getVertexShader();
    VkShaderModule fshader = program->getFragmentShader();

    // Create texture.
    // Note that STB does not provide a way to easily decode into a pre-allocated area, so we incur
    // two copies: one into the staging area at construction, and one into the device-only memory.
    LavaTexture* texture;
    {
        uint32_t width, height;
        int ok = stbi_info(TEXTURE_FILENAME, (int*) &width, (int*) &height, 0);
        if (!ok) {
            llog.error("{}: {}.", TEXTURE_FILENAME, stbi_failure_reason());
            std::terminate();
        }
        llog.info("Loading texture {:4}x{:4} {}", width, height, TEXTURE_FILENAME);
        uint8_t* texels = stbi_load(TEXTURE_FILENAME, (int*) &width, (int*) &height, 0, 4);
        texture = LavaTexture::create({
            .device = device, .gpu = gpu,
            .size = width * height * 4u,
            .source = texels,
            .width = width,
            .height = height,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
        });
        stbi_image_free(texels);
    }
    texture->uploadStage(context->beginWork());
    context->endWork();
    context->waitWork();
    texture->freeStage();
    VkImageView imageView = texture->getImageView();

    // Create the sampler.
    VkSampler sampler;
    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .minLod = 0.0f,
        .maxLod = 0.25f
    };
    vkCreateSampler(device, &samplerInfo, 0, &sampler);

    // Create the descriptor set.
    auto descriptors = LavaDescCache::create({
        .device = device,
        .uniformBuffers = {},
        .imageSamplers = { {
            .sampler = sampler,
            .imageView = imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        } }
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();
    const VkDescriptorSet dset = descriptors->getDescriptor();

    static_assert(sizeof(Vertex) == 16, "Unexpected vertex size.");
    auto pipelines = LavaPipeCache::create({
        .device = device,
        .descriptorLayouts = { dlayout },
        .renderPass = renderPass,
        .vshader = vshader,
        .fshader = fshader,
        .vertex = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .attributes = { {
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = 0u,
                .offset = 0u,
            }, {
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = 1u,
                .offset = 8u,
            } },
            .buffers = { {
                .binding = 0u,
                .stride = sizeof(Vertex),
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
    VkClearValue clearValue = { .color.float32 = {} };
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
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                &dset, 0, 0);

        // Make the draw call and end the render pass.
        vkCmdDraw(cmdbuffer, 4, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        context->endFrame();
    }

    // Wait for the command buffers to finish executing.
    context->waitFrame();

    // Cleanup.
    vkDestroySampler(device, sampler, 0);
    delete texture;
    delete descriptors;
    delete vertexBuffer;
    delete pipelines;
    delete program;
    delete context;
    return 0;
}
