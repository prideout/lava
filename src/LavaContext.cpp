// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <string>

#include "LavaInternal.h"

namespace par {
    struct LavaRecording {
        VkCommandBuffer cmd[2];
        VkFence fence[2];
        bool doneRecording[2];
        uint32_t currentIndex;
    };
}

using namespace par;
using namespace std;

static LavaVector<const char *> kRequiredExtensions {
    "VK_KHR_surface",
#if defined(__APPLE__)
    "VK_MVK_macos_surface",
#elif defined(__linux__)
    "VK_KHR_xcb_surface",
#endif
};

static LavaVector<const char *> kValidationLayers1 {
    "VK_LAYER_LUNARG_standard_validation"
};

static LavaVector<const char *> kValidationLayers2 {
    "VK_LAYER_GOOGLE_threading",       "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",  "VK_LAYER_LUNARG_image",
    "VK_LAYER_LUNARG_core_validation", "VK_LAYER_LUNARG_swapchain",
    "VK_LAYER_GOOGLE_unique_objects"
};

struct SwapchainBundle {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
    VkFramebuffer framebuffer;
    VkFence fence;
};

struct DepthBundle {
    VkImage image = 0;
    VkImageView view = 0;
    VkDeviceMemory mem = 0;
    VkFormat format = VK_FORMAT_D32_SFLOAT;
};

struct LavaContextImpl : LavaContext {
    explicit LavaContextImpl(bool useValidation) noexcept;
    ~LavaContextImpl() noexcept;
    void initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept;
    void killDevice() noexcept;
    VkCommandBuffer beginFrame() noexcept;
    void endFrame() noexcept;
    void initDepthBuffer() noexcept;
    bool determineMemoryType(uint32_t typeBits, VkFlags requirements,
            uint32_t *typeIndex) const noexcept;
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    VkPhysicalDevice mGpu = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mGpuProps;
    VkPhysicalDeviceFeatures mGpuFeatures;
    VkQueue mQueue;
    VkFormat mFormat;
    VkColorSpaceKHR mColorSpace;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    LavaVector<VkQueueFamilyProperties> mQueueProps;
    LavaVector<const char*> mEnabledExtensions;
    LavaVector<const char*> mEnabledLayers;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    SwapchainBundle mSwap[2] {};
    VkExtent2D mExtent;
    DepthBundle mDepth;
    VkSurfaceKHR mSurface;
    VkSemaphore mImageAvailable;
    VkSemaphore mDrawFinished;
    VkCommandBuffer mWorkCmd;
    VkFence mWorkFence;
    uint32_t mCurrentSwapIndex = ~0u;
    LavaRecording* mCurrentRecording = nullptr;
    VkDebugReportCallbackEXT mDebugCallback = nullptr;
};

namespace LavaLoader {
    bool init();
    void bind(VkInstance instance);
}

LAVA_DEFINE_UPCAST(LavaContext)

LavaContext* LavaContext::create(Config config) noexcept {
    auto impl = new LavaContextImpl(config.validation);
    impl->mSurface = config.createSurface(impl->mInstance);
    impl->initDevice(impl->mSurface, config.depthBuffer);
    return impl;
}

void LavaContext::operator delete(void* ptr) {
    auto impl = (LavaContextImpl*) ptr;
    ::delete impl;
}

LavaContextImpl::~LavaContextImpl() noexcept {
    killDevice();
    vkDestroyInstance(mInstance, VKALLOC);
}

static bool isExtensionSupported(const string& ext) noexcept;
static bool areAllLayersSupported(const LavaVector<VkLayerProperties>& props,
    const LavaVector<const char*>& layerNames) noexcept;

LavaContextImpl::LavaContextImpl(bool useValidation) noexcept {
    LavaLoader::init();
    // Form list of requested layers.
    LavaVector<VkLayerProperties> props;
    VkResult error = vkEnumerateInstanceLayerProperties(&props.size, nullptr);
    if (useValidation && !error && props.size > 0) {
        vkEnumerateInstanceLayerProperties(&props.size, props.alloc());
        if (areAllLayersSupported(props, kValidationLayers1)) {
            mEnabledLayers = kValidationLayers1;
        } else if (areAllLayersSupported(props, kValidationLayers2)) {
            mEnabledLayers = kValidationLayers2;
        }
    }
    for (auto layer : mEnabledLayers) {
        llog.info("Enabling instance layer {}.", layer);
    }

    // Form list of requested extensions.
    mEnabledExtensions = kRequiredExtensions;
    if (useValidation && isExtensionSupported(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
        llog.info("Enabling instance extension {}.", VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        mEnabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    // Create the instance.
    const VkApplicationInfo app {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Lava Application",
        .pEngineName = "Lava Engine",
        .apiVersion = VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = mEnabledLayers.size,
        .ppEnabledLayerNames = mEnabledLayers.data,
        .enabledExtensionCount = mEnabledExtensions.size,
        .ppEnabledExtensionNames = mEnabledExtensions.data,
    };
    error = vkCreateInstance(&info, VKALLOC, &mInstance);
    LOG_CHECK(not error, "Unable to create Vulkan instance.");
    LavaLoader::bind(mInstance);
}

void LavaContextImpl::killDevice() noexcept {
    vkDeviceWaitIdle(mDevice);
    destroyVma(mDevice);
    vkDestroyImageView(mDevice, mSwap[0].view, VKALLOC);
    vkDestroyImageView(mDevice, mSwap[1].view, VKALLOC);
    mSwap[0].view = mSwap[1].view = VK_NULL_HANDLE;

    // Technically the "if" is not needed because the Vulkan spec allows null here. However,
    // MoltenVK segfaults...
    if (mDepth.view) {
        vkDestroyImageView(mDevice, mDepth.view, VKALLOC);
        vkDestroyImage(mDevice, mDepth.image, VKALLOC);
        vkFreeMemory(mDevice, mDepth.mem, VKALLOC);
        mDepth.view = VK_NULL_HANDLE;
        mDepth.image = VK_NULL_HANDLE;
        mDepth.mem = VK_NULL_HANDLE;
    }

    vkDestroyRenderPass(mDevice, mRenderPass, VKALLOC);
    mRenderPass = VK_NULL_HANDLE;

    vkDestroyFence(mDevice, mSwap[0].fence, VKALLOC);
    vkDestroyFence(mDevice, mSwap[1].fence, VKALLOC);
    vkDestroyFence(mDevice, mWorkFence, VKALLOC);
    mSwap[0].fence = mSwap[1].fence = mWorkFence = VK_NULL_HANDLE;

    vkDestroySemaphore(mDevice, mImageAvailable, VKALLOC);
    vkDestroySemaphore(mDevice, mDrawFinished, VKALLOC);
    mImageAvailable = mDrawFinished = VK_NULL_HANDLE;

    vkDestroyFramebuffer(mDevice, mSwap[0].framebuffer, VKALLOC);
    vkDestroyFramebuffer(mDevice, mSwap[1].framebuffer, VKALLOC);
    mSwap[0].framebuffer = mSwap[1].framebuffer = VK_NULL_HANDLE;

    vkDestroySwapchainKHR(mDevice, mSwapchain, VKALLOC);
    mSwapchain = VK_NULL_HANDLE;

    VkCommandBuffer cmds[] = { mSwap[0].cmd, mSwap[1].cmd, mWorkCmd };
    vkFreeCommandBuffers(mDevice, mCommandPool, 3, cmds);
    mSwap[0].cmd = mSwap[1].cmd = mWorkCmd = VK_NULL_HANDLE;

    vkDestroyCommandPool(mDevice, mCommandPool, VKALLOC);
    mCommandPool = VK_NULL_HANDLE;

    if (mDebugCallback) {
        vkDestroyDebugReportCallbackEXT(mInstance, mDebugCallback, VKALLOC);
    }

    vkDestroyDevice(mDevice, VKALLOC);
    mDevice = VK_NULL_HANDLE;
}

void LavaContextImpl::initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept {
    assert(surface && "Missing VkSurfaceKHR instance.");
    // Pick the first physical device.
    LavaVector<VkPhysicalDevice> gpus;
    vkEnumeratePhysicalDevices(mInstance, &gpus.size, nullptr);
    VkResult error = vkEnumeratePhysicalDevices(mInstance, &gpus.size, gpus.alloc());
    LOG_CHECK(not error, "Unable to enumerate Vulkan devices.");
    mGpu = gpus[0];

    // We could call vkEnumerateDeviceExtensionProperties here to ensure that the platform-specific
    // swap chain extension is supported, but why bother? If it's not supported we'll find out
    // later, so go ahead and unconditionally add it to the list.
    mEnabledExtensions.clear();
    mEnabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Obtain various information about the GPU.
    vkGetPhysicalDeviceProperties(mGpu, &mGpuProps);
    vkGetPhysicalDeviceFeatures(mGpu, &mGpuFeatures);
    vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &mQueueProps.size, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &mQueueProps.size, mQueueProps.alloc());
    LOG_CHECK(mQueueProps.size > 0, "vkGetPhysicalDeviceQueueFamilyProperties error.");

    // Iterate over each queue to learn whether it supports presenting.
    const uint32_t queueCount = mQueueProps.size;
    vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(mGpu, i, surface, &supportsPresent[i]);
    }

    // Search for a graphics and present queue in the array of queue families.
    constexpr uint32_t NOT_FOUND = ~0u;
    uint32_t graphicsQueueNodeIndex = NOT_FOUND;
    uint32_t presentQueueNodeIndex = NOT_FOUND;
    for (uint32_t i = 0; i < queueCount; i++) {
        if (mQueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    LOG_CHECK(presentQueueNodeIndex != NOT_FOUND, "Can't find queue that supports "
        "both presentation and graphics.");

    // Create the VkDevice and queue.
    const float priorities[1] { 0 };
    const VkDeviceQueueCreateInfo queueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueNodeIndex,
        .queueCount = 1,
        .pQueuePriorities = priorities,
    };
    VkPhysicalDeviceFeatures features {};
    features.shaderClipDistance = mGpuFeatures.shaderClipDistance;
    VkDeviceCreateInfo deviceInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = mEnabledExtensions.size,
        .ppEnabledExtensionNames = mEnabledExtensions.data,
        .pEnabledFeatures = &features,
    };
    error = vkCreateDevice(mGpu, &deviceInfo, VKALLOC, &mDevice);
    LOG_CHECK(not error, "Unable to create Vulkan device.");
    vkGetPhysicalDeviceMemoryProperties(mGpu, &mMemoryProperties);
    vkGetDeviceQueue(mDevice, graphicsQueueNodeIndex, 0, &mQueue);

    // Debug callbacks.
    if (vkCreateDebugReportCallbackEXT) {
        VkDebugReportCallbackCreateInfoEXT cbinfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
            .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
            .pfnCallback = [](VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t,
                    size_t, int32_t, const char* pLayerPrefix, const char* pMessage,
                    void*) -> VkBool32 {
                if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
                    llog.error("VULKAN: ({}) {}", pLayerPrefix, pMessage);
                } else {
                    llog.warn("VULKAN: ({}) {}", pLayerPrefix, pMessage);
                }
                return VK_FALSE;
            }
        };
        vkCreateDebugReportCallbackEXT(mInstance, &cbinfo, VKALLOC, &mDebugCallback);
    }

    // Create the GPU memory allocator.
    createVma(mDevice, mGpu);

    // Get the list of formats that are supported:
    LavaVector<VkSurfaceFormatKHR> formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mGpu, surface, &formats.size, nullptr);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mGpu, surface, &formats.size, formats.alloc());
    LOG_CHECK(formats.size > 1, "Unable to find a surface format.");
    if (formats.size == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        mFormat = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        mFormat = formats[0].format;
    }
    mColorSpace = formats[0].colorSpace;

    // Create the command pool and command buffers.
    const VkCommandPoolCreateInfo poolinfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueNodeIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    error = vkCreateCommandPool(mDevice, &poolinfo, VKALLOC, &mCommandPool);
    LOG_CHECK(not error, "Unable to create command pool.");
    const VkCommandBufferAllocateInfo bufinfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 3,
    };
    VkCommandBuffer bufs[3];
    error = vkAllocateCommandBuffers(mDevice, &bufinfo, bufs);
    LOG_CHECK(not error, "Unable to allocate command buffers.");
    mSwap[0].cmd = bufs[0];
    mSwap[1].cmd = bufs[1];
    mWorkCmd = bufs[2];

    // Check the surface capabilities and formats.
    VkSurfaceCapabilitiesKHR surfCapabilities;
    error = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mGpu, surface, &surfCapabilities);
    LOG_CHECK(not error, "Unable to get surface caps.");

    LavaVector<VkPresentModeKHR> modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mGpu, surface, &modes.size, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mGpu, surface, &modes.size, modes.alloc());
    LOG_CHECK(modes.size > 0, "Unable to get present modes.");

    // Determine the size of the swap chain.
    mExtent = surfCapabilities.currentExtent;
    if (mExtent.width == 0xffffffff) {
        mExtent.width = 640;
        mExtent.height = 480;
        llog.warn("Platform surface does not have an extent, defaulting to {}x{}",
                mExtent.width, mExtent.height);
    }
    LOG_CHECK(mExtent.width >= surfCapabilities.minImageExtent.width &&
            mExtent.width <= surfCapabilities.maxImageExtent.width &&
            mExtent.height >= surfCapabilities.minImageExtent.height &&
            mExtent.height <= surfCapabilities.maxImageExtent.height,
            "Bad swap chain size.");
    LOG_CHECK(2 >= surfCapabilities.minImageCount && 2 <= surfCapabilities.maxImageCount,
            "Double buffering not supported.");

    // Create the VkSwapchainKHR
    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }
    const VkSwapchainCreateInfoKHR swapinfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = mFormat,
        .imageColorSpace = mColorSpace,
        .imageExtent = mExtent,
        .imageUsage = VkImageUsageFlags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
        .preTransform =  preTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = true,
    };
    error = vkCreateSwapchainKHR(mDevice, &swapinfo, VKALLOC, &mSwapchain);
    LOG_CHECK(not error, "Unable to create swap chain.");

    // Extract the VkImage objects from the swap chain.
    LavaVector<VkImage> images;
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &images.size, nullptr);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &images.size, images.alloc());
    LOG_CHECK(images.size > 0, "Unable to get swap chain images.");
    mSwap[0].image = images[0];
    mSwap[1].image = images[1];

    // Create the VkImageView objects.
    VkImageViewCreateInfo viewinfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .format = mFormat,
        .components = {
             .r = VK_COMPONENT_SWIZZLE_R,
             .g = VK_COMPONENT_SWIZZLE_G,
             .b = VK_COMPONENT_SWIZZLE_B,
             .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        },
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };
    for (int i = 0; i < 2; i++) {
        viewinfo.image = mSwap[i].image;
        error = vkCreateImageView(mDevice, &viewinfo, VKALLOC, &mSwap[i].view);
        LOG_CHECK(not error, "Unable to create swap chain image view.");
    }

    // Create the work fence and depth buffer.
    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkCreateFence(mDevice, &fenceInfo, VKALLOC, &mWorkFence);
    if (createDepthBuffer) {
        initDepthBuffer();
    }

    // Create the render pass used for drawing to the backbuffer.
    LavaVector<VkAttachmentDescription> rpattachments;
    rpattachments.push_back(VkAttachmentDescription {
         .format = mFormat,
         .samples = VK_SAMPLE_COUNT_1_BIT,
         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
         .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
         .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
         .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    });
    const VkAttachmentReference colorref {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depthref {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorref,
    };
    if (createDepthBuffer) {
        subpass.pDepthStencilAttachment = &depthref;
        rpattachments.push_back(VkAttachmentDescription {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }
    const VkRenderPassCreateInfo rpinfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = rpattachments.size,
        .pAttachments = rpattachments.data,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    error = vkCreateRenderPass(mDevice, &rpinfo, VKALLOC, &mRenderPass);
    LOG_CHECK(not error, "Unable to create render pass.");

    // Create two framebuffers (one for each element in the swap chain)
    LavaVector<VkImageView> fbattachments;
    fbattachments.push_back(mSwap[0].view);
    if (createDepthBuffer) {
        fbattachments.push_back(mDepth.view);
    }
    const VkFramebufferCreateInfo fbinfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mRenderPass,
        .attachmentCount = fbattachments.size,
        .pAttachments = fbattachments.data,
        .width = mExtent.width,
        .height = mExtent.height,
        .layers = 1,
    };
    error = vkCreateFramebuffer(mDevice, &fbinfo, VKALLOC, &mSwap[0].framebuffer);
    LOG_CHECK(not error, "Unable to create framebuffer.");
    fbattachments[0] = mSwap[1].view;
    error = vkCreateFramebuffer(mDevice, &fbinfo, VKALLOC, &mSwap[1].framebuffer);
    LOG_CHECK(not error, "Unable to create framebuffer.");

    // Create a fence for each command buffer.
    vkCreateFence(mDevice, &fenceInfo, VKALLOC, &mSwap[0].fence);
    vkCreateFence(mDevice, &fenceInfo, VKALLOC, &mSwap[1].fence);

    VkSemaphoreCreateInfo semaphoreInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(mDevice, &semaphoreInfo, VKALLOC, &mImageAvailable);
    vkCreateSemaphore(mDevice, &semaphoreInfo, VKALLOC, &mDrawFinished);
}

VkCommandBuffer LavaContextImpl::beginFrame() noexcept {
    // Wait for the previous submission of this command buffer to finish executing.
    vkWaitForFences(mDevice, 1, &mSwap[0].fence, VK_TRUE, ~0ull);
    vkResetFences(mDevice, 1, &mSwap[0].fence);
    // The given CPU fence and GPU semaphore will both be signaled when the presentation engine
    // releases the next available presentable image.
    uint32_t swapIndex;
    VkResult result = vkAcquireNextImageKHR(mDevice, mSwapchain, ~0ull, mImageAvailable,
             VK_NULL_HANDLE, &swapIndex);
    LOG_CHECK(result != VK_ERROR_OUT_OF_DATE_KHR, "Stale / resized swap chain not yet supported.");
    LOG_CHECK(result == VK_SUBOPTIMAL_KHR || result == VK_SUCCESS, "vkAcquireNextImageKHR error.");
    assert(swapIndex != mCurrentSwapIndex);
    mCurrentSwapIndex = swapIndex;
    // Start the command buffer.
    VkCommandBuffer cmdbuffer = mSwap[0].cmd;
    VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkResetCommandBuffer(cmdbuffer, 0);
    vkBeginCommandBuffer(cmdbuffer, &beginInfo);
    return cmdbuffer;
}

void LavaContextImpl::endFrame() noexcept {
    VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mImageAvailable,
        .pWaitDstStageMask = &waitDestStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &mSwap[0].cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mDrawFinished,
    };
    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mDrawFinished,
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain,
        .pImageIndices = &mCurrentSwapIndex,
    };
    vkEndCommandBuffer(mSwap[0].cmd);
    vkQueueSubmit(mQueue, 1, &submitInfo, mSwap[0].fence);
    vkQueuePresentKHR(mQueue, &presentInfo);
    std::swap(mSwap[0], mSwap[1]);
}


void LavaContextImpl::initDepthBuffer() noexcept {
    const VkImageCreateInfo image {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = mDepth.format,
        .extent = {mExtent.width, mExtent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    };
    VkMemoryAllocateInfo memalloc {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    };
    VkResult error = vkCreateImage(mDevice, &image, VKALLOC, &mDepth.image);
    LOG_CHECK(not error, "Unable to create depth image.");

    VkMemoryRequirements memreqs;
    vkGetImageMemoryRequirements(mDevice, mDepth.image, &memreqs);

    memalloc.allocationSize = memreqs.size;
    bool pass = determineMemoryType(memreqs.memoryTypeBits, 0, &memalloc.memoryTypeIndex);
    LOG_CHECK(pass, "Unable to determine memory type.");

    error = vkAllocateMemory(mDevice, &memalloc, VKALLOC, &mDepth.mem);
    LOG_CHECK(not error, "Unable to allocate depth image.");

    error = vkBindImageMemory(mDevice, mDepth.image, mDepth.mem, 0);
    LOG_CHECK(not error, "Unable to bind depth image.");

    const VkImageViewCreateInfo viewinfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .format = mDepth.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
        .image = mDepth.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };
    error = vkCreateImageView(mDevice, &viewinfo, VKALLOC, &mDepth.view);
    LOG_CHECK(not error, "Unable to create depth view.");

    VkCommandBuffer cmd = this->beginWork();
    const VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = mDepth.image,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    this->endWork();
    this->waitWork();
}

bool LavaContextImpl::determineMemoryType(uint32_t typeBits, VkFlags requirements,
        uint32_t *typeIndex) const noexcept {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if (typeBits & 1) {
            const VkFlags typeFlags = mMemoryProperties.memoryTypes[i].propertyFlags;
            if ((typeFlags & requirements) == requirements) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

VkCommandBuffer LavaContext::beginFrame() noexcept {
    return upcast(this)->beginFrame();
}

void LavaContext::endFrame() noexcept {
    upcast(this)->endFrame();
}

VkInstance LavaContext::getInstance() const noexcept {
    return upcast(this)->mInstance;
}

VkExtent2D LavaContext::getSize() const noexcept {
    return upcast(this)->mExtent;
}

VkDevice LavaContext::getDevice() const noexcept {
    return upcast(this)->mDevice;
}

VkPhysicalDevice LavaContext::getGpu() const noexcept {
    return upcast(this)->mGpu;
}

const VkPhysicalDeviceFeatures& LavaContext::getGpuFeatures() const noexcept {
    return upcast(this)->mGpuFeatures;
}

VkQueue LavaContext::getQueue() const noexcept {
    return upcast(this)->mQueue;
}

VkFormat LavaContext::getFormat() const noexcept {
    return upcast(this)->mFormat;
}

VkColorSpaceKHR LavaContext::getColorSpace() const noexcept {
    return upcast(this)->mColorSpace;
}

const VkPhysicalDeviceMemoryProperties& LavaContext::getMemoryProperties() const noexcept {
    return upcast(this)->mMemoryProperties;
}

VkRenderPass LavaContext::getRenderPass() const noexcept {
    return upcast(this)->mRenderPass;
}

VkSwapchainKHR LavaContext::getSwapchain() const noexcept {
    return upcast(this)->mSwapchain;
}

VkImage LavaContext::getImage(uint32_t i) const noexcept {
    return upcast(this)->mSwap[i].image;
}

VkImageView LavaContext::getImageView(uint32_t i) const noexcept {
    return upcast(this)->mSwap[i].view;
}

VkFramebuffer LavaContext::getFramebuffer(uint32_t i) const noexcept {
    return upcast(this)->mSwap[i].framebuffer;
}

void LavaContext::waitFrame(int n) noexcept {
    auto impl = upcast(this);
    if (n < 0) {
        const VkFence fences[] = {impl->mSwap[0].fence, impl->mSwap[1].fence};
        vkWaitForFences(impl->mDevice, 2, fences, VK_TRUE, ~0ull);
    } else {
        vkWaitForFences(impl->mDevice, 1, &impl->mSwap[n].fence, VK_TRUE, ~0ull);
    }
}

VkCommandBuffer LavaContext::beginWork() noexcept {
    auto impl = upcast(this);
    vkWaitForFences(impl->mDevice, 1, &impl->mWorkFence, VK_TRUE, ~0ull);
    vkResetFences(impl->mDevice, 1, &impl->mWorkFence);
    VkCommandBuffer cmdbuffer = impl->mWorkCmd;
    VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkResetCommandBuffer(cmdbuffer, 0);
    vkBeginCommandBuffer(cmdbuffer, &beginInfo);
    return cmdbuffer;
}

void LavaContext::endWork() noexcept {
    auto impl = upcast(this);
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &impl->mWorkCmd,
    };
    vkEndCommandBuffer(impl->mWorkCmd);
    vkQueueSubmit(impl->mQueue, 1, &submitInfo, impl->mWorkFence);
}

void LavaContext::waitWork() noexcept {
    auto impl = upcast(this);
    vkWaitForFences(impl->mDevice, 1, &impl->mWorkFence, VK_TRUE, ~0ull);
}

LavaRecording* LavaContext::createRecording() noexcept {
    auto impl = upcast(this);
    LavaRecording* recording = new LavaRecording();
    const VkCommandBufferAllocateInfo bufinfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = impl->mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 2,
    };
    vkAllocateCommandBuffers(impl->mDevice, &bufinfo, recording->cmd);
    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkCreateFence(impl->mDevice, &fenceInfo, VKALLOC, &recording->fence[0]);
    vkCreateFence(impl->mDevice, &fenceInfo, VKALLOC, &recording->fence[1]);
    recording->doneRecording[0] = false;
    recording->doneRecording[1] = false;
    recording->currentIndex = ~0u;
    return recording;
}

void LavaContext::freeRecording(LavaRecording* recording) noexcept {
    auto impl = upcast(this);
    assert(recording);
    vkDestroyFence(impl->mDevice, recording->fence[0], VKALLOC);
    vkDestroyFence(impl->mDevice, recording->fence[1], VKALLOC);
    vkFreeCommandBuffers(impl->mDevice, impl->mCommandPool, 2, recording->cmd);
    delete recording;
}

VkCommandBuffer LavaContext::beginRecording(LavaRecording* recording, uint32_t i) noexcept {
    auto impl = upcast(this);
    assert(recording && i < 2);
    impl->mCurrentRecording = recording;
    recording->currentIndex = i;
    VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(recording->cmd[i], &beginInfo);
    return recording->cmd[i];
}

void LavaContext::endRecording() noexcept {
    auto impl = upcast(this);
    assert(impl->mCurrentRecording);
    LavaRecording* recording = impl->mCurrentRecording;
    impl->mCurrentRecording = nullptr;
    const uint32_t index = recording->currentIndex;
    vkEndCommandBuffer(recording->cmd[index]);
    recording->doneRecording[index] = true;
    recording->currentIndex = 1 - index;
}

void LavaContext::presentRecording(LavaRecording* recording) noexcept {
    auto impl = upcast(this);
    assert(recording && recording->doneRecording[0] && recording->doneRecording[1]);
    constexpr VkPipelineStageFlags waitDestStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    uint32_t index = 0;
    vkAcquireNextImageKHR(impl->mDevice, impl->mSwapchain, ~0ull, impl->mImageAvailable,
             VK_NULL_HANDLE, &index);
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &impl->mImageAvailable,
        .pWaitDstStageMask = &waitDestStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &recording->cmd[index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &impl->mDrawFinished,
    };
    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &impl->mDrawFinished,
        .swapchainCount = 1,
        .pSwapchains = &impl->mSwapchain,
        .pImageIndices = &index,
    };
    VkFence fence = recording->fence[index];
    vkWaitForFences(impl->mDevice, 1, &fence, VK_TRUE, ~0ull);
    vkResetFences(impl->mDevice, 1, &fence);
    vkQueueSubmit(impl->mQueue, 1, &submitInfo, fence);
    vkQueuePresentKHR(impl->mQueue, &presentInfo);
}

void LavaContext::waitRecording(LavaRecording* recording) noexcept {
    auto impl = upcast(this);
    assert(recording && recording->doneRecording[0] && recording->doneRecording[1]);
    vkWaitForFences(impl->mDevice, 2, recording->fence, VK_TRUE, ~0ull);
}

static bool isExtensionSupported(const string& ext) noexcept {
    LavaVector<VkExtensionProperties> props;
    vkEnumerateInstanceExtensionProperties(nullptr, &props.size, nullptr);
    VkResult error = vkEnumerateInstanceExtensionProperties(nullptr, &props.size, props.alloc());
    LOG_CHECK(not error, "Unable to enumerate extension properties.");
    for (auto prop : props) {
        if (ext != prop.extensionName) {
            return true;
        }
    }
    return false;
}

static bool areAllLayersSupported(const LavaVector<VkLayerProperties>& props,
        const LavaVector<const char*>& layerNames) noexcept {
    for (auto layerName : layerNames) {
        bool found = false;
        for (auto prop : props) {
            if (!strcmp(layerName, prop.layerName)) {
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}
