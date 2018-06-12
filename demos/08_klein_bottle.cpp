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

    struct Uniforms {
        Matrix4 mvp;
        Matrix3 imv;
        float time;
    };

    constexpr char const* BACKDROP_VSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec2 uv;
    layout(location = 0) out vec2 vert_uv;
    void main() {
        gl_Position = position;
        gl_Position.z = 0.99;
        vert_uv = uv;
    })";

    constexpr char const* BACKDROP_FSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec2 vert_uv;
    layout(binding = 1) uniform sampler2D img;
    void main() {
        frag_color = texture(img, vert_uv);
    })";

    constexpr char const* KLEIN_VSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) in vec4 position;
    layout(location = 1) in vec2 uv;
    layout(location = 0) out vec2 vert_uv;
    layout(binding = 0) uniform Uniforms {
        mat4 mvp;
        mat3 imv;
        float time;
    };
    void main() {
        gl_Position = mvp * position;
        vert_uv = uv;
    })";

    constexpr char const* KLEIN_FSHADER = AMBER_PREFIX_450 R"(
    layout(location = 0) out vec4 frag_color;
    layout(location = 0) in vec2 vert_uv;
    layout(binding = 1) uniform sampler2D img;
    void main() {
        vec2 uv = vert_uv;
        uv.y = 1.0 - uv.y;
        frag_color = texture(img, uv);
    })";

    struct Vertex {
        float position[3];
        float uv[2];
    };

    #define P +1
    #define N -1
    const Vertex BACKDROP_VERTICES[] {
        {{P, N, 0}, {1,1}},
        {{N, N, 0}, {0,1}},
        {{P, P, 0}, {1,0}},
        {{N, P, 0}, {0,0}},
    };
    #undef N
    #undef P

    struct Geometry {
        std::unique_ptr<LavaGpuBuffer> vertices;
        std::unique_ptr<LavaGpuBuffer> indices;
        std::unique_ptr<LavaCpuBuffer> vstage;
        std::unique_ptr<LavaCpuBuffer> istage;
        VkBufferCopy vregion = {};
        VkBufferCopy iregion = {};
        size_t nvertices;
        size_t ntriangles;
    };

    template <typename T>
    unique_ptr<T> make_unique(typename T::Config cfg) {
        return unique_ptr<T>(T::create(cfg));
    }
}

static Geometry load_geometry(const char* filename, VkDevice device, VkPhysicalDevice gpu) {
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
    llog.info("\tshape 0 has {} triangles, {} verts, {} texcoords",
            shapes[0].mesh.indices.size() / 3,
            attrib.vertices.size() / 3,
            attrib.texcoords.size() / 2);

    vector<uint16_t> indices;
    const auto& obj_indices = shapes[0].mesh.indices;
    indices.reserve(obj_indices.size());
    for (size_t i = 0; i < obj_indices.size();) {
        const auto& a = obj_indices[i++];
        const auto& b = obj_indices[i++];
        const auto& c = obj_indices[i++];
        indices.emplace_back(a.vertex_index);
        indices.emplace_back(b.vertex_index);
        indices.emplace_back(c.vertex_index);
        assert(a.vertex_index == a.texcoord_index);
        assert(b.vertex_index == b.texcoord_index);
        assert(c.vertex_index == c.texcoord_index);
    }
    for (auto compound_index : shapes[0].mesh.indices) {
        indices.emplace_back(compound_index.vertex_index);
        assert(compound_index.vertex_index == compound_index.texcoord_index);
    }

    assert(sizeof(attrib.vertices[0] == sizeof(float)));
    const uint32_t vsize = (uint32_t) (sizeof(float) * (attrib.vertices.size() +
            attrib.texcoords.size()));
    const uint32_t isize = (uint32_t) (sizeof(uint16_t) * indices.size());

    Geometry geo {
        .vertices = make_unique<LavaGpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = vsize,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        }),
        .indices = make_unique<LavaGpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = isize,
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        }),
        .vstage = make_unique<LavaCpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = vsize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        }),
        .istage = make_unique<LavaCpuBuffer>({
            .device = device,
            .gpu = gpu,
            .size = isize,
            .source = indices.data(),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        }),
        .vregion = { .size = vsize },
        .iregion = { .size = isize },
        .nvertices = attrib.vertices.size() / 3,
        .ntriangles = shapes[0].mesh.indices.size() / 3,
    };

    geo.vstage->setData(attrib.vertices.data(), sizeof(float) * attrib.vertices.size());
    geo.vstage->setData(attrib.texcoords.data(), sizeof(float) * attrib.texcoords.size(),
            sizeof(float) * attrib.vertices.size());

    return geo;
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
    auto geo = load_geometry("../extras/assets/klein.obj", device, gpu);

    // Start uploading the textures and geometry.
    VkCommandBuffer workbuf = context->beginWork();
    backdrop_texture->uploadStage(workbuf);
    occlusion->uploadStage(workbuf);
    rust->uploadStage(workbuf);
    vkCmdCopyBuffer(workbuf, geo.istage->getBuffer(), geo.indices->getBuffer(), 1, &geo.iregion);
    vkCmdCopyBuffer(workbuf, geo.vstage->getBuffer(), geo.vertices->getBuffer(), 1, &geo.vregion);

    // Create shader modules.
    auto make_program = [device](char const* vshader, char const* fshader) {
        auto ptr = AmberProgram::create(vshader, fshader);
        ptr->compile(device);
        return unique_ptr<AmberProgram>(ptr);
    };
    auto backdrop_program = make_program(BACKDROP_VSHADER, BACKDROP_FSHADER);
    auto klein_program = make_program(KLEIN_VSHADER, KLEIN_FSHADER);

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
    LavaCpuBuffer::Config cfg {
        .device = device, .gpu = gpu, .size = sizeof(Uniforms),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
    unique_ptr<LavaCpuBuffer> ubo[2] = {
        make_unique<LavaCpuBuffer>(cfg),
        make_unique<LavaCpuBuffer>(cfg)
    };

    // Describe the vertex configuration for all geometries.
    const LavaPipeCache::VertexState backdrop_vertex {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .attributes = { {
            .format = VK_FORMAT_R32G32B32_SFLOAT,
        }, {
            .format = VK_FORMAT_R32G32_SFLOAT,
            .location = 1u,
            .offset = 12u,
        } },
        .buffers = { {
            .stride = 20,
        } }
    };
    const LavaPipeCache::VertexState klein_bottle_vertex {
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .attributes = { {
            .binding = 0u,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .location = 0u,
            .offset = 0u,
        }, {
            .binding = 1u,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .location = 1u,
            .offset = 0u,
        } },
        .buffers = { {
            .binding = 0u,
            .stride = 12,
        }, {
            .binding = 1u,
            .stride = 8u,
        } }
    };

    // Create the descriptor cache.
    auto descriptors = make_unique<LavaDescCache>({
        .device = device,
        .uniformBuffers = {{}},
        .imageSamplers = {{}}
    });
    const VkDescriptorSetLayout dlayout = descriptors->getLayout();

    // Create the pipeline cache.
    static_assert(sizeof(Vertex) == 20, "Unexpected vertex size.");
    auto pipelines = make_unique<LavaPipeCache>({
        .device = device,
        .descriptorLayouts = { dlayout },
        .renderPass = renderPass
    });
    const VkPipelineLayout playout = pipelines->getLayout();

    // Wait until done uploading the textures and geometry.
    context->endWork();
    context->waitWork();
    backdrop_texture->freeStage();
    occlusion->freeStage();
    rust->freeStage();
    delete vboStage;
    geo.istage.reset();
    geo.vstage.reset();

    // Fill in some info structs before starting the render loop.
    const VkClearValue clearValues[] = {
        { .color.float32 = {0.1, 0.2, 0.4, 1.0} },
        { .depthStencil = {1, 0} }
    };
    const VkViewport viewport = {
        .width = (float) extent.width,
        .height = (float) extent.height,
        .maxDepth = 1.0
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
        swap(ubo[0], ubo[1]);

        // Draw the backdrop.
        descriptors->setImageSampler(1, {
            .sampler = sampler,
            .imageView = backdrop_texture->getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                descriptors->getDescPointer(), 0, 0);
        pipelines->setVertexState(backdrop_vertex);
        pipelines->setVertexShader(backdrop_program->getVertexShader());
        pipelines->setFragmentShader(backdrop_program->getFragmentShader());
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
        vkCmdBindVertexBuffers(cmd, 0, 1, backdrop_vertices->getBufferPtr(), &zero_offset);
        vkCmdDraw(cmd, 4, 1, 0, 0);

        // Draw the klein bottle.
        descriptors->setImageSampler(1, {
            .sampler = sampler,
            .imageView = occlusion->getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                descriptors->getDescPointer(), 0, 0);
        pipelines->setVertexState(klein_bottle_vertex);
        pipelines->setVertexShader(klein_program->getVertexShader());
        pipelines->setFragmentShader(klein_program->getFragmentShader());
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines->getPipeline());
        const VkBuffer buffers[] { geo.vertices->getBuffer(), geo.vertices->getBuffer()};
        const VkDeviceSize offsets[] { 0, geo.nvertices * sizeof(float) * 3 };
        vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, geo.indices->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, geo.ntriangles * 3, 1, 0, 0, 0);

        vkCmdEndRenderPass(cmd);
        context->endRecording();
    }

    // See https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
    constexpr Matrix4 vkcorrection {
        1.0,  0.0, 0.0, 0.0,
        0.0, -1.0, 0.0, 0.0,
        0.0,  0.0, 0.5, 0.5,
        0.0,  0.0, 0.0, 1.0,
    };
    constexpr float h = 0.5f;
    constexpr float w = h * DEMO_WIDTH / DEMO_HEIGHT;
    constexpr float znear = 3;
    constexpr float zfar = 10;
    constexpr float y = 0.6;
    constexpr Point3 eye {0, y, -7};
    constexpr Point3 target {0, y, 0};
    constexpr Vector3 up {0, 1, 0};

    // Main render loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Matrix4 projection = M4MakeFrustum(-w, w, -h, h, znear, zfar);
        projection = M4Mul(vkcorrection, projection);
        Matrix4 view = M4MakeLookAt(eye, target, up);
        Matrix4 model = M4MakeIdentity();
        Matrix4 modelview = M4Mul(view, model);
        Matrix4 mvp = M4Mul(projection, modelview);
        Uniforms uniforms {
            .mvp = mvp,
            .imv = M4GetUpper3x3(modelview),
            .time = (float) glfwGetTime()
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
