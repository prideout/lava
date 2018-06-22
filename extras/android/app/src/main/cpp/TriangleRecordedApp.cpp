#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaPipeCache.h>
#include <par/LavaLog.h>
#include <par/LavaContext.h>

#include <par/AmberApplication.h>
#include <par/AmberProgram.h>

#include <math.h>

using namespace std;
using namespace par;

namespace {
    const std::string vertShaderGLSL = AMBER_PREFIX R"GLSL(
    layout(location=0) in vec2 position;
    layout(location=1) in vec4 color;
    layout(location=0) out vec4 vert_color;
    void main() {
        gl_Position = vec4(position, 0, 1);
        vert_color = color;
    })GLSL";

    const std::string fragShaderGLSL = AMBER_PREFIX R"GLSL(
    layout(location=0) out lowp vec4 frag_color;
    layout(location=0) in highp vec4 vert_color;
    void main() {
        frag_color = vert_color;
    })GLSL";

    struct Vertex {
        float position[2];
        uint32_t color;
    };

    constexpr float PI = 3.1415926535;
    const Vertex TRIANGLE_VERTICES[] {
        {{1, 0}, 0xffff0000u},
        {{cosf(PI * 2 / 3), sinf(PI * 2 / 3)}, 0xff00ff00u},
        {{cosf(PI * 4 / 3), sinf(PI * 4 / 3)}, 0xff0000ffu},
    };

    struct TriangleRecordedApp : AmberApplication {
        TriangleRecordedApp(SurfaceFn createSurface);
        ~TriangleRecordedApp();
        void draw(double seconds) override;
        LavaContext* mContext;
        AmberProgram* mProgram;
        LavaGpuBuffer* mVertexBuffer;
        LavaRecording* mRecording;
        LavaPipeCache* mPipelines;
    };
}

TriangleRecordedApp::TriangleRecordedApp(SurfaceFn createSurface) {
    // Create the instance, device, swap chain, and command buffers.
    mContext = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .createSurface = createSurface
    });
    const auto device = mContext->getDevice();
    const auto gpu = mContext->getGpu();
    const auto renderPass = mContext->getRenderPass();
    const auto extent = mContext->getSize();

    // Begin populating a vertex buffer.
    mVertexBuffer = LavaGpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    auto stage = LavaCpuBuffer::create({
        .device = device,
        .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    VkCommandBuffer cmdbuffer = mContext->beginWork();
    const VkBufferCopy region = { .size = sizeof(TRIANGLE_VERTICES) };
    vkCmdCopyBuffer(cmdbuffer, stage->getBuffer(), mVertexBuffer->getBuffer(), 1, &region);
    mContext->endWork();

    // Compile shaders.
    mProgram = AmberProgram::create(vertShaderGLSL, fragShaderGLSL);
    if (!mProgram->compile(device)) {
        terminate();
    }

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    mPipelines = LavaPipeCache::create({
        .device = device,
        .descriptorLayouts = {},
        .renderPass = renderPass,
        .vshader = mProgram->getVertexShader(),
        .fshader = mProgram->getFragmentShader(),
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
    VkPipeline pipeline = mPipelines->getPipeline();

    // Finish populating the vertex buffer.
    mContext->waitWork();
    delete stage;

    // Fill in some structs that will be used when rendering.
    const VkClearValue clearValue = { .color.float32 = {0.1, 0.2, 0.4, 1.0} };
    const VkViewport viewport = {
        .width = (float) extent.width,
        .height = (float) extent.height
    };
    const VkRect2D scissor { .extent = extent };
    const VkBuffer buffer[] = { mVertexBuffer->getBuffer() };
    const VkDeviceSize offsets[] = { 0 };

    // Record two command buffers.
    mRecording = mContext->createRecording();
    for (uint32_t i = 0; i < 2; i++) {
        const VkCommandBuffer cmdbuffer = mContext->beginRecording(mRecording, i);
        const VkRenderPassBeginInfo rpbi {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .framebuffer = mContext->getFramebuffer(i),
            .renderPass = renderPass,
            .renderArea.extent = extent,
            .pClearValues = &clearValue,
            .clearValueCount = 1
        };
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        mContext->endRecording();
    }
}

TriangleRecordedApp::~TriangleRecordedApp() {
    mContext->waitRecording(mRecording);
    mContext->freeRecording(mRecording);
    delete mPipelines;
    delete mProgram;
    delete mVertexBuffer;
    delete mContext;
}

void TriangleRecordedApp::draw(double time) {
    mContext->presentRecording(mRecording);
}

static AmberApplication::Register app("trianglerecorded", [] (AmberApplication::SurfaceFn cb) {
    return new TriangleRecordedApp(cb);
});
