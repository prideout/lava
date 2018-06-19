#include <par/LavaLoader.h>
#include <par/LavaLog.h>
#include <par/LavaContext.h>

#include <par/AmberApplication.h>

using namespace std;
using namespace par;

struct ClearScreenApp : AmberApplication {
    ClearScreenApp(SurfaceFn createSurface);
    ~ClearScreenApp();
    void draw(double seconds) override;
    LavaContext* mContext = nullptr;
};

ClearScreenApp::ClearScreenApp(SurfaceFn createSurface) {
    mContext = LavaContext::create({
        .depthBuffer = false,
        .validation = true,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .createSurface = createSurface
    });
    llog.info("LavaContext created.");
}

ClearScreenApp::~ClearScreenApp() {
    delete mContext;
}

void ClearScreenApp::draw(double seconds) {
    VkCommandBuffer cmdbuffer = mContext->beginFrame();
    const float red = fmod(seconds, 1.0);
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

static AmberApplication::Register app("clearscreen", [] (AmberApplication::SurfaceFn cb) {
    return new ClearScreenApp(cb);
});
