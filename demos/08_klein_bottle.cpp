// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaDescCache.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>
#include <par/LavaPipeCache.h>
#include <par/LavaTexture.h>
#include <par/AmberProgram.h>

#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "vmath.h"

using namespace par;
using namespace std;

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
    constexpr int DEMO_WIDTH = 512;
    constexpr int DEMO_HEIGHT = 512;

    constexpr char const* BACKDROP_VSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec2 uv;
    layout(location = 0) out vec2 vert_uv;
    void main() {
        gl_Position = vec4(position, 1);
        vert_uv = uv;
    })";

    constexpr char const* BACKDROP_FSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec2 vert_uv;
    layout(binding = 0) uniform sampler2D img;
    void main() {
        // frag_color = vec4(vert_uv, 0, 1);
        frag_color = texture(img, vert_uv);
    })";

    struct Vertex {
        float position[3];
        float uv[2];
    };

    #define P +1
    #define N -1
    const Vertex BACKDROP_VERTICES[] {
        {{P, P, 0}, {1,1}},
        {{N, P, 0}, {0,1}},
        {{P, N, 0}, {1,0}},
        {{N, N, 0}, {0,0}},
    };
    #undef N
    #undef P

    struct Mesh {
        std::unique_ptr<LavaGpuBuffer> vertices;
        std::unique_ptr<LavaGpuBuffer> indices;
    };

    template <typename T>
    unique_ptr<T> make_unique(typename T::Config cfg) {
        return unique_ptr<T>(T::create(cfg));
    }
}

static Mesh load_mesh(const char* filename, VkDevice device, VkPhysicalDevice gpu) {
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    string err;
    constexpr bool triangulate = true;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, nullptr, triangulate);
    if (!err.empty()) {
        llog.error("{}: {}.", filename, err);
        return {};
    }
    if (!ret || shapes.size() == 0) {
        llog.error("Failed to load {}.", filename);
        return {};
    }
    llog.info("Loaded {:2} shapes from {}", shapes.size(), filename);
    llog.info("\tshape 0 has {} triangles", shapes[0].mesh.indices.size() / 3);
    
    Mesh mesh = {
        .vertices = make_unique<LavaGpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = (uint32_t) (sizeof(uint16_t) * shapes[0].mesh.indices.size()),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        }),
        .indices = make_unique<LavaGpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = (uint32_t) (sizeof(float) * attrib.vertices.size()),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        })
    };

    return mesh;
}

static unique_ptr<LavaTexture> load_texture(char const* filename, VkDevice device,
        VkPhysicalDevice gpu) {
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
    return unique_ptr<LavaTexture>(texture);
}

static void run_demo(LavaContext* context, GLFWwindow* window) {
    const VkDevice device = context->getDevice();
    const VkPhysicalDevice gpu = context->getGpu();
    const VkRenderPass renderPass = context->getRenderPass();
    const VkExtent2D extent = context->getSize();

    // Load textures from disk.
    auto backdrop_texture = load_texture("../extras/assets/abstract.jpg", device, gpu);
    auto occlusion = load_texture("../extras/assets/klein.png", device, gpu);
    auto rust = load_texture("../extras/assets/rust.png", device, gpu);

    // Create the klein bottle mesh.
    auto mesh = load_mesh("../extras/assets/klein.obj", device, gpu);

    // Start uploading the textures and geometry.
    VkCommandBuffer workbuf = context->beginWork();
    backdrop_texture->uploadStage(workbuf);
    occlusion->uploadStage(workbuf);
    rust->uploadStage(workbuf);

    // Create shader modules.
    auto make_program = [device](char const* vshader, char const* fshader) {
        auto ptr = AmberProgram::create(vshader, fshader);
        ptr->compile(device);
        return unique_ptr<AmberProgram>(ptr);
    };
    auto backdrop_program = make_program(BACKDROP_VSHADER, BACKDROP_FSHADER);

    // Create the backdrop mesh.
    auto backdrop_vertices = make_unique<LavaGpuBuffer>({
        .device = device,
        .gpu = gpu,
        .size = sizeof(BACKDROP_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    auto vboStage = LavaCpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(BACKDROP_VERTICES),
        .source = BACKDROP_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    const VkBufferCopy region = { .size = sizeof(BACKDROP_VERTICES) };
    vkCmdCopyBuffer(workbuf, vboStage->getBuffer(), backdrop_vertices->getBuffer(), 1, &region);

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
    auto descriptors = make_unique<LavaDescCache>({
        .device = device,
        .uniformBuffers = {},
        .imageSamplers = { {
            .sampler = sampler,
            .imageView = backdrop_texture->getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        } }
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();
    const VkDescriptorSet dset = descriptors->getDescriptor();

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 20, "Unexpected vertex size.");
    auto pipelines = make_unique<LavaPipeCache>({
        .device = device,
        .vertex = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .attributes = { {
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .location = 0u,
                .offset = 0u,
            }, {
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = 1u,
                .offset = 12u,
            } },
            .buffers = { {
                .binding = 0u,
                .stride = 20,
            } }
        },
        .descriptorLayouts = { dlayout },
        .vshader = backdrop_program->getVertexShader(),
        .fshader = backdrop_program->getFragmentShader(),
        .renderPass = renderPass
    });
    const VkPipeline pipeline = pipelines->getPipeline();
    const VkPipelineLayout playout = pipelines->getLayout();

    // Wait until done uploading the textures and geometry.
    context->endWork();
    context->waitWork();
    backdrop_texture->freeStage();
    occlusion->freeStage();
    rust->freeStage();
    delete vboStage;

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
        const VkBuffer buffer[] = { backdrop_vertices->getBuffer() };
        const VkDeviceSize offsets[] = { 0 };
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                &dset, 0, 0);
        vkCmdDraw(cmdbuffer, 4, 1, 0, 0);
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

    vkDestroySampler(device, sampler, 0);
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
        .depthBuffer = true,
        .validation = true,
        .createSurface = [window] (VkInstance instance) {
            VkSurfaceKHR surface;
            glfwCreateWindowSurface(instance, window, nullptr, &surface);
            return surface;
        }
    });
    run_demo(context, window);
    delete context;
    return 0;
}
