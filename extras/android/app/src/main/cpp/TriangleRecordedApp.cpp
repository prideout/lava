#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaLog.h>
#include <par/LavaContext.h>

#include <par/AmberApplication.h>
#include <par/AmberProgram.h>

#include <math.h>

using namespace std;
using namespace par;

namespace {
    constexpr float PI = 3.1415926535;

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

    const Vertex TRIANGLE_VERTICES[] {
        {{1, 0}, 0xffff0000u},
        {{cosf(PI * 2 / 3), sinf(PI * 2 / 3)}, 0xff00ff00u},
        {{cosf(PI * 4 / 3), sinf(PI * 4 / 3)}, 0xff0000ffu},
    };
}

struct TriangleRecordedApp : AmberApplication {
    TriangleRecordedApp(SurfaceFn createSurface);
    ~TriangleRecordedApp();
    void draw(double seconds) override;
    LavaContext* mContext;
    AmberProgram* mProgram;
    LavaGpuBuffer* mVertexBuffer;
};

TriangleRecordedApp::TriangleRecordedApp(SurfaceFn createSurface) {
    // Create the instance, device, swap chain, and command buffers.
    mContext = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .createSurface = createSurface
    });
    auto device = mContext->getDevice();
    auto gpu = mContext->getGpu();

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

    // Finish populating the vertex buffer.
    mContext->waitWork();
    delete stage;
}

TriangleRecordedApp::~TriangleRecordedApp() {
    delete mProgram;
    delete mVertexBuffer;
    delete mContext;
}

void TriangleRecordedApp::draw(double time) {
    VkCommandBuffer cmdbuffer = mContext->beginFrame();
    const float red = fmod(time, 1.0);
    const VkClearValue clearValue = { .color.float32 = {red, 0, 0, 1} };
    const VkRenderPassBeginInfo rpbi {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .framebuffer = mContext->getFramebuffer(),
        .renderPass = mContext->getRenderPass(),
        .renderArea.extent.width = mContext->getSize().width,
        .renderArea.extent.height = mContext->getSize().height,
        .pClearValues = &clearValue,
        .clearValueCount = 1
    };
    vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    // ...do not draw any geometry...

    // End the render pass, flush the command buffer, and present the backbuffer.
    vkCmdEndRenderPass(cmdbuffer);
    mContext->endFrame();
}

static AmberApplication::Register app("trianglerecorded", [] (AmberApplication::SurfaceFn cb) {
    return new TriangleRecordedApp(cb);
});
