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
#include <stb_image.h>
#include <GLFW/glfw3.h>

#include "vmath.h"

using namespace par;

#define STRINGIFY(x) #x
#define STRINGIFY_(x) STRINGIFY(x)
#define SHADER_PREFIX "#version 450\n#line " STRINGIFY_(__LINE__) "\n"

namespace {
    constexpr int DEMO_WIDTH = 512;
    constexpr int DEMO_HEIGHT = 512;

    const std::string vertShaderGLSL = SHADER_PREFIX R"(
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec2 uv;
    layout(location = 0) out vec2 vert_uv;
    void main() {
        gl_Position = vec4(position, 0, 1);
        vert_uv = uv;
    })";

    const std::string fragShaderGLSL = SHADER_PREFIX R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec2 vert_uv;
    layout(binding = 0) uniform sampler2D img;
    void main() {
        // frag_color = vec4(vert_uv, 0, 1);
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
        .size = sizeof(VERTICES),
        .source = VERTICES,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    });

    // Compile shaders.
    auto program = AmberProgram::create(vertShaderGLSL, fragShaderGLSL);
    VkShaderModule vshader = program->getVertexShader(device);
    VkShaderModule fshader = program->getFragmentShader(device);

    // Create texture.
    LavaTexture* texture;
    {
        const char* TEXTURE_FILENAME = "../extras/assets/abstract.jpg";
        int w, h, nchan;
        int ok = stbi_info(TEXTURE_FILENAME, &w, &h, &nchan);
        if (!ok) {
            llog.error("{}: {}.", TEXTURE_FILENAME, stbi_failure_reason());
            exit(1);
        }
        llog.info("Loading texture {:4}x{:4} {}", w, h, TEXTURE_FILENAME);
        uint8_t* texels = stbi_load(TEXTURE_FILENAME, &w, &h, &nchan, 4);
        assert(texels);
        texture = LavaTexture::create({
            .device = device, .gpu = gpu,
            .size = (uint32_t) (w * h * 4),
            .source = texels,
            .info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .extent = { (uint32_t) w, (uint32_t) h, 1 },
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .mipLevels = 1,
                .arrayLayers = 1,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .samples = VK_SAMPLE_COUNT_1_BIT,
            }
        });
        const auto& props = texture->getProperties();
        stbi_image_free(texels);
        texture->uploadStage(context->beginWork());
        context->endWork();
        context->waitWork();
        texture->freeStage();
    }
    const auto& textureProps = texture->getProperties();

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
            .imageView = textureProps.view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        } }
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();
    const VkDescriptorSet dset = descriptors->getDescriptorSet();

    static_assert(sizeof(Vertex) == 16, "Unexpected vertex size.");
    auto pipelines = LavaPipeCache::create({
        .device = device,
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
    LavaTexture::destroy(&texture);
    LavaDescCache::destroy(&descriptors);
    LavaCpuBuffer::destroy(&vertexBuffer);
    AmberProgram::destroy(&program, device);
    LavaPipeCache::destroy(&pipelines);
    LavaContext::destroy(&context);
    return 0;
}
