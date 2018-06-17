#include <par/LavaLog.h>
#include <par/LavaContext.h>

#include "AmberApp.h"

using namespace std;
using namespace par;

struct TriangleRecordedApp : AmberApp {
    TriangleRecordedApp(SurfaceFn createSurface);
    ~TriangleRecordedApp();
    void draw(double seconds) override;
    LavaContext* mContext = nullptr;
};

TriangleRecordedApp::TriangleRecordedApp(SurfaceFn createSurface) {
    mContext = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .createSurface = createSurface
    });
    llog.info("LavaContext created.");
}

TriangleRecordedApp::~TriangleRecordedApp() {
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

// AmberApp* AmberApp::create(int appIndex, SurfaceFn createSurface) {
//     return new TriangleRecordedApp(createSurface);
// }