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
    constexpr int DEMO_WIDTH = 640 / 2;
    constexpr int DEMO_HEIGHT = 718 / 2;
    constexpr int NUM_PARTICLES = 100000;

    float global_time = 0;
    bool global_refresh_signal = false;

    struct Uniforms {
        float time;
        float npoints;
    };

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
        terminate();
    }
    llog.info("Loading texture from {} ({}x{})", filename, width, height);
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
    #define BLUENOISE_BASEURL "https://prideout.net/assets/"
    #define BLUENOISE_FILENAME "bluenoise.trimmed.bin"
    #define BLUENOISE_URL BLUENOISE_BASEURL BLUENOISE_FILENAME
    if (access(BLUENOISE_FILENAME, F_OK) == -1) {
        llog.info("Downloading {}", BLUENOISE_FILENAME);
        par_easycurl_to_file(BLUENOISE_URL, BLUENOISE_FILENAME);
    }

    // Load the Gibbons
    auto gibbons_pts = [device, gpu, context](char const* filename) {

        llog.info("Decoding Gibbons texture");
        int width, height;
        uint8_t* pixels = stbi_load(filename, (int*) &width, (int*) &height, 0, 1);
        assert(pixels);

        llog.info("Generating {} points", NUM_PARTICLES);
        auto bluenoise = par_bluenoise_from_file(BLUENOISE_FILENAME, NUM_PARTICLES);
        par_bluenoise_density_from_gray(bluenoise, pixels, width, height, 1);
        float* pts = par_bluenoise_generate_exact(bluenoise, NUM_PARTICLES, 2);
        int j = NUM_PARTICLES - 1;
        for (int i = 0; i < NUM_PARTICLES / 2; i++, j--) {
            swap(pts[i * 2], pts[j * 2]);
            swap(pts[i * 2 + 1], pts[j * 2 + 1]);
            pts[i * 2 + 1] *= -1;
        }

        llog.info("Uploading {} points to GPU", NUM_PARTICLES);
        const uint32_t bufsize = sizeof(float) * 2 * NUM_PARTICLES;
        for (int i = 0; i < 4; i++) {
            llog.debug("\t{: .3f} {: .3f}", pts[i*2], pts[i*2+1]);
        }
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
            .source = pts,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        });
        const VkBufferCopy region { .size = bufsize };
        VkCommandBuffer workbuf = context->beginWork();
        vkCmdCopyBuffer(workbuf, stage->getBuffer(), vbo->getBuffer(), 1, &region);
        context->endWork();
        context->waitWork();
        par_bluenoise_free(bluenoise);

        return vbo;
    }("../extras/assets/particles3.jpg");

    // Load textures from disk.
    VkCommandBuffer workbuf = context->beginWork();
    auto particles2_texture = load_texture("../extras/assets/particles3.jpg", device, gpu);
    particles2_texture->uploadStage(workbuf);

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
    unique_ptr<LavaCpuBuffer> ubo[2];
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

    // Describe the vertex configuration for all geometries.
    const LavaPipeCache::VertexState backdrop_vertex {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .attributes = { {
            .format = VK_FORMAT_R32G32_SFLOAT,
        }, {
            .format = VK_FORMAT_R32G32_SFLOAT,
            .location = 1u,
            .offset = 8u,
        } },
        .buffers = { {
            .stride = 16,
        } }
    };
    const LavaPipeCache::VertexState points_vertex {
        .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        .attributes = { {
            .format = VK_FORMAT_R32G32_SFLOAT,
        }, {
            .binding = 1u,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .location = 1u,
        } },
        .buffers = { {
            .stride = 8,
        },  {
            .binding = 1u,
            .stride = 8,
        } }
    };

    // Create the pipeline.
    auto pipelines = make_unique<LavaPipeCache>({
        .device = device,
        .descriptorLayouts = { dlayout },
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
    const VkDeviceSize zero_offsets[2] {};
    const VkBuffer ptbuffers[2] { gibbons_pts->getBuffer(), gibbons_pts->getBuffer() };
    auto raster_state = pipelines->getDefaultRasterState();
    raster_state.blending.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    raster_state.blending.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    // Set up some UI handlers.
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action != GLFW_PRESS) {
            return;
        }
        switch (key) {
            case GLFW_KEY_RIGHT:
                global_time += 0.1;
                break;
            case GLFW_KEY_LEFT:
                global_time -= 0.1;
                break;
            case GLFW_KEY_SPACE:
                global_refresh_signal = true;
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
        }
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double dx, double dy) {
        global_time = std::max(0.0, global_time + dx * 0.1);
    });

    // Create shader modules.
    auto make_program = [device](string vshader, string fshader) {
        const string vs = AmberProgram::getChunk(__FILE__, vshader);
        const string fs = AmberProgram::getChunk(__FILE__, fshader);
        auto ptr = AmberProgram::create(vs, fs);
        if (!ptr->compile(device)) {
            delete ptr;
            ptr = nullptr; 
        }
        return unique_ptr<AmberProgram>(ptr);
    };
    auto backdrop_program = make_program("backdrop.vs", "backdrop.fs");
    backdrop_program->watchDirectory("../demos", [] (const string& filename) {
        llog.warn("{} has been modified", filename);
        global_refresh_signal = true;
    });

    while (!glfwWindowShouldClose(window)) {

        auto points_program = make_program("points.vs", "points.fs");
        if (!points_program) {
            sleep(1);
            continue;
        }

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
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1, &dset, 0,
                    0);
            swap(ubo[0], ubo[1]);

            // Draw the backdrop.
            raster_state.blending.blendEnable = VK_FALSE;
            pipelines->setRasterState(raster_state);
            pipelines->setVertexState(backdrop_vertex);
            pipelines->setVertexShader(backdrop_program->getVertexShader());
            pipelines->setFragmentShader(backdrop_program->getFragmentShader());
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
            vkCmdBindVertexBuffers(cmd, 0, 1, backdrop_vertices->getBufferPtr(), &zero_offset);
            vkCmdDraw(cmd, 4, 1, 0, 0);

            // Draw the points.
            raster_state.blending.blendEnable = VK_TRUE;
            pipelines->setRasterState(raster_state);
            pipelines->setVertexState(points_vertex);
            pipelines->setVertexShader(points_program->getVertexShader());
            pipelines->setFragmentShader(points_program->getFragmentShader());
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
            vkCmdBindVertexBuffers(cmd, 0, 2, ptbuffers, zero_offsets);
            vkCmdDraw(cmd, NUM_PARTICLES, 1, 0, 0);

            vkCmdEndRenderPass(cmd);
            context->endRecording();
        }

        llog.info("Starting animation");
        const double start = getCurrentTime();
        float seconds_elapsed = 1;
        global_refresh_signal = false;

        // Execute the render loop, logging every second until interactive mode is enabled.
        while (!glfwWindowShouldClose(window) && !global_refresh_signal) {
            glfwPollEvents();
            double now = getCurrentTime() - start;
            global_time = now;
            if (global_time > seconds_elapsed) {
                llog.debug("\t{} seconds", seconds_elapsed++);
            }
            Uniforms uniforms {
                .time = global_time,
                .npoints = (float) NUM_PARTICLES
            };
            ubo[0]->setData(&uniforms, sizeof(uniforms));
            swap(ubo[0], ubo[1]);
            context->presentRecording(frame);
            backdrop_program->checkDirectory();
        }

        // Wait for the command buffer to finish before deleting any Vulkan objects.
        context->waitRecording(frame);

        // Cleanup. All Vulkan objects except the sampler and recorded command buffers are stored
        // unique_ptr so they self-destruct when the scope ends.
        context->freeRecording(frame);
    }

    vkDestroySampler(device, sampler, 0);
}

int main(const int argc, const char *argv[]) {
    GLFWwindow* window;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(DEMO_WIDTH, DEMO_HEIGHT, "gibbons", 0, 0);
    LavaContext* context = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
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

#if 0
-- backdrop.vs -------------------------------------------------------------------------------------

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec2 vert_uv;
void main() {
    gl_Position = vec4(position, 1);
    vert_uv = uv;
}

-- backdrop.fs -------------------------------------------------------------------------------------

layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec2 vert_uv;
layout(binding = 1) uniform sampler2D img;
layout(binding = 0) uniform Uniforms {
    float original_time;
    float npoints;
};
void main() {
    float recording_delay = 0.5;
    float time = max(0.0, original_time - recording_delay);

    frag_color = vec4(0.92);
    // vec4 tex_color = texture(img, vert_uv);
    // float t = clamp(time - 5.0, 0.0, 1.0);
    // frag_color = mix(frag_color, tex_color, t);
}

-- points.vs ---------------------------------------------------------------------------------------

layout(location = 1) in vec2 gibbons_position;
layout(location = 0) out vec4 vert_color;
layout(binding = 0) uniform Uniforms {
    float original_time;
    float npoints;
};
layout(binding = 1) uniform sampler2D img;

void main() {
    float recording_delay = 0.5;
    float time = max(0.0, original_time - recording_delay);

    float a = 5.0 - time;
    gl_PointSize = 3.0; // clamp(a, 2.0, 5.0);

    float aspect = 640.0 / 718.0;
    vec2 gibbons = gibbons_position * vec2(2.25, 2.0);
    float n = float(gl_VertexIndex) / npoints;

    float t = 3.14 * 2.0 * n * 0.6;
    float t2 = 1.0 * (t + time);
    float s2 = sin(t2);
    vec2 pt = -0.05 * vec2(16 * s2*s2*s2, 13*cos(t2)-5*cos(2*t2)-2*cos(3*t2)-cos(4*t2));
    pt += vec2(0.0, -0.1);

    t = clamp((time - n * 10.0) * 0.5, 0.02, 1.0);
    pt = mix(pt, gibbons, t);

    gl_Position = vec4(pt, 0, 1);
    vec3 red = vec3(0); // vec3(0.6, 0.2, 0.2);
    vert_color = vec4(red, 1);
    vert_color.a = min(0.04 + t, 0.3);

    vert_color.a *= 1.0 - n;
}

-- points.fs ---------------------------------------------------------------------------------------

layout(location = 0) out vec4 frag_color;
layout(location = 0) in vec4 vert_color;
void main() {
    frag_color = vert_color;
}

----------------------------------------------------------------------------------------------------
#endif