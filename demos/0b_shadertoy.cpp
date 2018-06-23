//    1794 x 1080
//    Full screen triangle
//    https://www.shadertoy.com/view/4dfXDn

#include <par/LavaLoader.h>
#include <par/LavaCpuBuffer.h>
#include <par/LavaDescCache.h>
#include <par/LavaGpuBuffer.h>
#include <par/LavaPipeCache.h>
#include <par/LavaLog.h>
#include <par/LavaContext.h>

#include <par/AmberApplication.h>
#include <par/AmberProgram.h>

#include "vmath.h"

using namespace std;
using namespace par;

namespace {

    struct Vertex {
        float position[2];
        uint32_t color;
    };

    const Vertex TRIANGLE_VERTICES[] {
        {{-1, -1}, 0xffff0000u},
        {{ 3, -1}, 0xff00ff00u},
        {{-1,  3}, 0xff0000ffu},
    };

    struct ShaderToyApp : AmberApplication {
        ShaderToyApp(SurfaceFn createSurface);
        ~ShaderToyApp();
        void draw(double seconds) override;
        LavaContext* mContext;
        AmberProgram* mProgram;
        LavaGpuBuffer* mVertexBuffer;
        LavaRecording* mRecording;
        LavaPipeCache* mPipelines;
        LavaDescCache* mDescriptors;
        LavaCpuBuffer* mUniforms[2];
        Matrix4 mProjection;
    };
}

ShaderToyApp::ShaderToyApp(SurfaceFn createSurface) {
    // Create the instance, device, swap chain, and command buffers.
    mContext = LavaContext::create({
        .depthBuffer = false, .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT, .createSurface = createSurface
    });
    const auto device = mContext->getDevice();
    const auto gpu = mContext->getGpu();
    const auto renderPass = mContext->getRenderPass();
    const auto extent = mContext->getSize();
    llog.info("Surface size: {}x{}", extent.width, extent.height);

    // Compute a projection that makes [-1,+1] fit.
    float hw = (float) extent.width / extent.height;
    float hh = 1;
    mProjection = M4MakeOrthographic(-hw, hw, -hh, hh, -1, 1);

    // Begin populating a vertex buffer.
    mVertexBuffer = LavaGpuBuffer::create({
        .device = device, .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    });
    auto stage = LavaCpuBuffer::create({
        .device = device, .gpu = gpu,
        .size = sizeof(TRIANGLE_VERTICES), .source = TRIANGLE_VERTICES,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    VkCommandBuffer cmdbuffer = mContext->beginWork();
    const VkBufferCopy region = { .size = sizeof(TRIANGLE_VERTICES) };
    vkCmdCopyBuffer(cmdbuffer, stage->getBuffer(), mVertexBuffer->getBuffer(), 1, &region);
    mContext->endWork();

    // Compile shaders.

    auto make_program = [device](string vshader, string fshader) {
        const string vs = AmberProgram::getChunk(__FILE__, vshader);
        const string fs = AmberProgram::getChunk(__FILE__, fshader);
        auto ptr = AmberProgram::create(vs, fs);
        if (!ptr->compile(device)) {
            delete ptr;
            ptr = nullptr; 
        }
        return ptr;
    };

    mProgram = make_program("shadertoy.vs", "shadertoy.fs");
    if (!mProgram) {
        terminate();
    }

    // Create the double-buffered UBO.
    LavaCpuBuffer::Config cfg {
        .device = device, .gpu = gpu, .size = sizeof(Matrix4),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
    mUniforms[0] = LavaCpuBuffer::create(cfg);
    mUniforms[1] = LavaCpuBuffer::create(cfg);

    // Create the descriptor set.
    mDescriptors = LavaDescCache::create({
        .device = device, .uniformBuffers = { 0 }, .imageSamplers = {}
    });
    const VkDescriptorSetLayout dlayout = mDescriptors->getLayout();

    // Create the pipeline.
    static_assert(sizeof(Vertex) == 12, "Unexpected vertex size.");
    mPipelines = LavaPipeCache::create({
        .device = device, .descriptorLayouts = { dlayout }, .renderPass = renderPass,
        .vshader = mProgram->getVertexShader(), .fshader = mProgram->getFragmentShader(),
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
    VkPipelineLayout playout = mPipelines->getLayout();

    // Finish populating the vertex buffer.
    mContext->waitWork();
    delete stage;

    // Fill in some structs that will be used when rendering.
    const VkClearValue clearValue = { .color.float32 = {} };
    const VkViewport viewport = {
        .width = (float) extent.width, .height = (float) extent.height
    };
    const VkRect2D scissor { .extent = extent };
    const VkBuffer buffer[] = { mVertexBuffer->getBuffer() };
    const VkDeviceSize offsets[] = { 0 };

    // Record two command buffers.
    mRecording = mContext->createRecording();
    for (uint32_t i = 0; i < 2; i++) {
        const VkRenderPassBeginInfo rpbi {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .framebuffer = mContext->getFramebuffer(i),
            .renderPass = renderPass,
            .renderArea.extent = extent,
            .pClearValues = &clearValue,
            .clearValueCount = 1
        };
        mDescriptors->setUniformBuffer(0, mUniforms[i]->getBuffer());
        const VkDescriptorSet dset = mDescriptors->getDescriptor();

        const VkCommandBuffer cmdbuffer = mContext->beginRecording(mRecording, i);
        vkCmdBeginRenderPass(cmdbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, buffer, offsets);
        vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, playout, 0, 1,
                &dset, 0, 0);
        vkCmdDraw(cmdbuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdbuffer);
        mContext->endRecording();
    }
}

ShaderToyApp::~ShaderToyApp() {
    mContext->waitRecording(mRecording);
    mContext->freeRecording(mRecording);
    delete mUniforms[0];
    delete mUniforms[1];
    delete mDescriptors;
    delete mPipelines;
    delete mProgram;
    delete mVertexBuffer;
    delete mContext;
}

void ShaderToyApp::draw(double time) {
    Matrix4 matrix = M4Mul(mProjection, M4MakeRotationZ(time));
    mUniforms[0]->setData(&matrix, sizeof(matrix));
    mContext->presentRecording(mRecording);
    swap(mUniforms[0], mUniforms[1]);
}

static AmberApplication::Register app("shadertoy", [] (AmberApplication::SurfaceFn cb) {
    return new ShaderToyApp(cb);
});

#if 0
-- shadertoy.vs ------------------------------------------------------------------------------------

layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
layout(location=0) out highp vec2 uv;
layout(binding = 0) uniform MatrixBlock {
    mat4 transform;
};
void main() {
    gl_Position = vec4(position, 0, 1);
    uv = position.xy;
}

-- shadertoy.fs ------------------------------------------------------------------------------------

precision mediump int;
precision highp float;

layout(location=0) out lowp vec4 frag_color;
layout(location=0) in highp vec2 uv;
void main() {
    frag_color = vec4(uv, 0, 1);
}

----------------------------------------------------------------------------------------------------
#endif