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

#include <chrono>

using namespace par;
using namespace std;

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define PAR_EASYCURL_IMPLEMENTATION
#include <par_easycurl.h>

#define PAR_BLUENOISE_IMPLEMENTATION
#include <par_bluenoise.h>

namespace {
    constexpr int DEMO_WIDTH = 640;
    constexpr int DEMO_HEIGHT = 797;
    constexpr int NUM_PARTICLES = 20000;

    struct Uniforms {
        float time;
        float npoints;
    };

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
    layout(binding = 1) uniform sampler2D img;
    void main() {
        frag_color = mix(texture(img, vert_uv), vec4(1), 0.9);
    })";

    constexpr char const* POINTS_VSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) in vec2 position;
    layout(location = 0) out vec4 vert_color;
    layout(binding = 0) uniform Uniforms {
        float time;
        float npoints;
    };
    void main() {
        gl_Position = vec4(position * vec2(1.5, -1.25), 0, 1);
        gl_PointSize = 4.0;
        vert_color = vec4(1, 1, 1, 0);
        if (gl_VertexIndex < int(npoints * time / 3.0)) {
            vert_color = vec4(0, 0, 0, 0.3);
        }
    })";

    constexpr char const* POINTS_FSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec4 vert_color;
    void main() {
        frag_color = vert_color;
    })";

    struct Vertex {
        float position[2];
        float uv[2];
    };

    #define P +1
    #define N -1
    const Vertex BACKDROP_VERTICES[] {
        {{P, P}, {1,1}},
        {{N, P}, {0,1}},
        {{P, N}, {1,0}},
        {{N, N}, {0,0}},
    };
    #undef N
    #undef P

    template <typename T>
    unique_ptr<T> make_unique(typename T::Config cfg) {
        return unique_ptr<T>(T::create(cfg));
    }

    double getCurrentTime() {
        static auto start = chrono::high_resolution_clock::now();
        auto now = chrono::high_resolution_clock::now();
        return 0.001 * chrono::duration_cast<chrono::milliseconds>(now - start).count();
    }
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

    // Fetch the bluenoise data.
    par_easycurl_init(0);
    #define BLUENOISE_BASEURL "http://github.prideout.net/assets/"
    #define BLUENOISE_FILENAME "bluenoise.trimmed.bin"
    #define BLUENOISE_URL BLUENOISE_BASEURL BLUENOISE_FILENAME
    if (access(BLUENOISE_FILENAME, F_OK) == -1) {
        llog.info("Downloading {}", BLUENOISE_FILENAME);
        par_easycurl_to_file(BLUENOISE_URL, BLUENOISE_FILENAME);
    }
    auto bluenoise = par_bluenoise_from_file(BLUENOISE_FILENAME, 0);

    // Load the P ♥ R
    auto pheartr = [bluenoise, device, gpu, context](char const* filename) {

        llog.info("Decoding mask texture");
        int width, height;
        uint8_t* pixels = stbi_load(filename, (int*) &width, (int*) &height, 0, 1);
        assert(pixels);
        llog.info("Loaded mask {:4}x{:4} {}", width, height, filename);

        llog.info("Pushing density function");
        par_bluenoise_density_from_gray(bluenoise, pixels, width, height, 1);
        stbi_image_free(pixels);

        llog.info("Generating {} points", NUM_PARTICLES);
        const float* cpupts = par_bluenoise_generate_exact(bluenoise, NUM_PARTICLES, 2);
        const uint32_t bufsize = 2 * sizeof(float) * NUM_PARTICLES;
        for (int i = 0; i < 4; i++) {
            llog.debug("\t{: .3f} {: .3f}", cpupts[i*2], cpupts[i*2+1]);
        }

        llog.info("Uploading {} points to GPU", NUM_PARTICLES);
        auto vbo = make_unique<LavaGpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = bufsize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        });
        auto stage = make_unique<LavaCpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = bufsize,
            .source = cpupts,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        });
        const VkBufferCopy region { .size = bufsize };
        VkCommandBuffer workbuf = context->beginWork();
        vkCmdCopyBuffer(workbuf, stage->getBuffer(), vbo->getBuffer(), 1, &region);
        context->endWork();
        context->waitWork();

        return vbo;
    }("../extras/assets/particles1.png");
    par_bluenoise_free(bluenoise);

    // Load textures from disk.
    VkCommandBuffer workbuf = context->beginWork();
    auto particles2_texture = load_texture("../extras/assets/particles2.jpg", device, gpu);
    particles2_texture->uploadStage(workbuf);

    // Create shader modules.
    auto make_program = [device](char const* vshader, char const* fshader) {
        auto ptr = AmberProgram::create(vshader, fshader);
        ptr->compile(device);
        return unique_ptr<AmberProgram>(ptr);
    };
    auto backdrop_program = make_program(BACKDROP_VSHADER, BACKDROP_FSHADER);
    auto points_program = make_program(POINTS_VSHADER, POINTS_FSHADER);

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

    // Create the double-buffered UBO.
    std::unique_ptr<LavaCpuBuffer> ubo[2];
    for (int i = 0; i < 2; i++) {
        ubo[i] = make_unique<LavaCpuBuffer>({
            .device = device, .gpu = gpu, .size = sizeof(Uniforms),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        });
    }

    // Create the descriptor set.
    auto descriptors = make_unique<LavaDescCache>({
        .device = device,
        .uniformBuffers = { 0 },
        .imageSamplers = { {
            .sampler = sampler,
            .imageView = particles2_texture->getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        } }
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();
    const VkDescriptorSet dset = descriptors->getDescriptor();

    // Describe the vertex configuration for all geometries.
    const LavaPipeCache::VertexState backdrop_vertex {
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
            .stride = 16,
        } }
    };
    const LavaPipeCache::VertexState points_vertex {
        .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        .attributes = { {
            .binding = 0u,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .location = 0u,
            .offset = 0u,
        } },
        .buffers = { {
            .binding = 0u,
            .stride = 8,
        } }
    };

    // Create the pipeline.
    auto pipelines = make_unique<LavaPipeCache>({
        .device = device,
        .vertex = {},
        .descriptorLayouts = { dlayout },
        .vshader = {},
        .fshader = {},
        .renderPass = renderPass
    });
    const VkPipelineLayout playout = pipelines->getLayout();

    // Wait until done uploading the textures and geometry.
    context->endWork();
    context->waitWork();
    particles2_texture->freeStage();
    delete vboStage;

    // Fill in some info structs before starting the render loop.
    const VkClearValue clearValues[] = {
        { .color.float32 = {0.1, 0.2, 0.4, 1.0} }
    };
    const VkViewport viewport = {
        .width = (float) extent.width,
        .height = (float) extent.height
    };
    const VkRect2D scissor { .extent = extent };
    VkRenderPassBeginInfo rpbi {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .renderArea.extent = extent,
        .pClearValues = clearValues,
        .clearValueCount = 2
    };
    const VkDeviceSize zero_offset {};
    constexpr auto VSHADER = VK_SHADER_STAGE_VERTEX_BIT;
    constexpr auto FSHADER = VK_SHADER_STAGE_FRAGMENT_BIT;
    auto raster_state = pipelines->getDefaultRasterState();
    raster_state.blending.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    raster_state.blending.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    // Record two command buffers.
    LavaRecording* frame = context->createRecording();
    for (uint32_t i = 0; i < 2; i++) {
        rpbi.framebuffer = context->getFramebuffer(i);
        const VkCommandBuffer cmd = context->beginRecording(frame, i);

        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Push uniforms.
        descriptors->setUniformBuffer(0, ubo[0]->getBuffer());
        VkDescriptorSet dset = descriptors->getDescriptor();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1, &dset, 0, 0);
        swap(ubo[0], ubo[1]);

        // Draw the backdrop.
        raster_state.blending.blendEnable = VK_FALSE;
        pipelines->setRasterState(raster_state);
        pipelines->setVertexState(backdrop_vertex);
        pipelines->setShaderModule(VSHADER, backdrop_program->getVertexShader());
        pipelines->setShaderModule(FSHADER, backdrop_program->getFragmentShader());
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
        vkCmdBindVertexBuffers(cmd, 0, 1, backdrop_vertices->getBufferPtr(), &zero_offset);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Draw the points.
        raster_state.blending.blendEnable = VK_TRUE;
        pipelines->setRasterState(raster_state);
        pipelines->setVertexState(points_vertex);
        pipelines->setShaderModule(VSHADER, points_program->getVertexShader());
        pipelines->setShaderModule(FSHADER, points_program->getFragmentShader());
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
        vkCmdBindVertexBuffers(cmd, 0, 1, pheartr->getBufferPtr(), &zero_offset);
        vkCmdDraw(cmd, NUM_PARTICLES, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
        context->endRecording();
    }

    // Main render loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        Uniforms uniforms {
            .time = (float) fmod(getCurrentTime(), 3.0),
            .npoints = (float) NUM_PARTICLES
        };
        ubo[0]->setData(&uniforms, sizeof(uniforms));
        swap(ubo[0], ubo[1]);

        context->presentRecording(frame);
    }

    // Wait for the command buffer to finish before deleting any Vulkan objects.
    context->waitRecording(frame);

    // Cleanup. All Vulkan objects except the sampler and recorded command buffers are stored
    // unique_ptr so they self-destruct when the scope ends.
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
        .depthBuffer = false,
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
