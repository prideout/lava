// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaContext.h>
#include <par/LavaLog.h>

#include <string_view>

#include "LavaInternal.h"

using namespace par;

static LavaVector<const char *> kRequiredExtensions {
    "VK_KHR_surface",
    "VK_MVK_macos_surface",
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
};

struct DepthBundle {
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;
    VkFormat format = VK_FORMAT_D16_UNORM;
};

class LavaContextImpl : public LavaContext {
public:
    LavaContextImpl(bool useValidation) noexcept;
    ~LavaContextImpl() noexcept;
    void initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept;
    void killDevice() noexcept;
    void swap() noexcept;
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
    SwapchainBundle mSwap[2] = {};
    VkExtent2D mExtent;
    DepthBundle mDepth;
    friend class LavaContext;
};

namespace LavaLoader {
    bool init();
    void bind(VkInstance instance);
}

LAVA_IMPL_CLASS(LavaContext)

LavaContext* LavaContext::create(bool useValidation) noexcept {
    return new LavaContextImpl(useValidation);
}

void LavaContext::destroy(LavaContext** that) noexcept {
    delete upcast(*that);
    *that = nullptr;
}

static bool isExtensionSupported(std::string_view ext) noexcept;
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
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Lava Application",
        .pEngineName = "Lava Engine",
        .apiVersion = VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledLayerCount = mEnabledLayers.size,
        .ppEnabledLayerNames = mEnabledLayers.data,
        .enabledExtensionCount = mEnabledExtensions.size,
        .ppEnabledExtensionNames = mEnabledExtensions.data,
    };
    error = vkCreateInstance(&info, VKALLOC, &mInstance);
    LOG_CHECK(!error, "Unable to create Vulkan instance.");
    LavaLoader::bind(mInstance);
}

LavaContextImpl::~LavaContextImpl() noexcept {
    vkDestroyInstance(mInstance, VKALLOC);
}

void LavaContextImpl::killDevice() noexcept {
    vkDestroyImageView(mDevice, mSwap[0].view, VKALLOC);
    vkDestroyImageView(mDevice, mSwap[1].view, VKALLOC);
    mSwap[0].view = mSwap[1].view = VK_NULL_HANDLE;

    vkDestroyImageView(mDevice, mDepth.view, VKALLOC);
    vkDestroyImage(mDevice, mDepth.image, VKALLOC);
    vkFreeMemory(mDevice, mDepth.mem, VKALLOC);
    mDepth.view = VK_NULL_HANDLE;
    mDepth.image = VK_NULL_HANDLE;
    mDepth.mem = VK_NULL_HANDLE;

    vkDestroyRenderPass(mDevice, mRenderPass, VKALLOC);
    mRenderPass = VK_NULL_HANDLE;

    vkDestroyFramebuffer(mDevice, mSwap[0].framebuffer, VKALLOC);
    vkDestroyFramebuffer(mDevice, mSwap[1].framebuffer, VKALLOC);
    mSwap[0].framebuffer = mSwap[1].framebuffer = VK_NULL_HANDLE;

    vkDestroySwapchainKHR(mDevice, mSwapchain, VKALLOC);
    mSwapchain = VK_NULL_HANDLE;

    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mSwap[0].cmd);
    vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mSwap[1].cmd);
    mSwap[0].cmd = mSwap[1].cmd = VK_NULL_HANDLE;

    vkDestroyCommandPool(mDevice, mCommandPool, VKALLOC);
    mCommandPool = VK_NULL_HANDLE;
    mSwap[0].cmd = mSwap[1].cmd = VK_NULL_HANDLE;

    vkDestroyDevice(mDevice, VKALLOC);
    mDevice = VK_NULL_HANDLE;
}

void LavaContextImpl::initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept {
    assert(surface && "Missing VkSurfaceKHR instance.");
    // Pick the first physical device.
    LavaVector<VkPhysicalDevice> gpus;
    vkEnumeratePhysicalDevices(mInstance, &gpus.size, nullptr);
    VkResult error = vkEnumeratePhysicalDevices(mInstance, &gpus.size, gpus.alloc());
    LOG_CHECK(!error, "Unable to enumerate Vulkan devices.");
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
    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(mGpu, i, surface, &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both.
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueCount; i++) {
        if (mQueueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    LOG_CHECK(presentQueueNodeIndex != UINT32_MAX, "Can't find queue that supports "
        "both presentation and graphics.");

    // Create the VkDevice and queue.
    const float priorities[1] = { 0 };
    const VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueNodeIndex,
        .queueCount = 1,
        .pQueuePriorities = priorities,
    };
    VkPhysicalDeviceFeatures features = {};
    features.shaderClipDistance = mGpuFeatures.shaderClipDistance;
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = mEnabledExtensions.size,
        .ppEnabledExtensionNames = mEnabledExtensions.data,
        .pEnabledFeatures = &features,
    };
    error = vkCreateDevice(mGpu, &deviceInfo, VKALLOC, &mDevice);
    LOG_CHECK(!error, "Unable to create Vulkan device.");
    vkGetPhysicalDeviceMemoryProperties(mGpu, &mMemoryProperties);
    vkGetDeviceQueue(mDevice, graphicsQueueNodeIndex, 0, &mQueue);

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
    const VkCommandPoolCreateInfo poolinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueNodeIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    error = vkCreateCommandPool(mDevice, &poolinfo, VKALLOC, &mCommandPool);
    LOG_CHECK(!error, "Unable to create command pool.");
    const VkCommandBufferAllocateInfo bufinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 2,
    };
    VkCommandBuffer bufs[2];
    error = vkAllocateCommandBuffers(mDevice, &bufinfo, bufs);
    LOG_CHECK(!error, "Unable to allocate command buffers.");
    mSwap[0].cmd = bufs[0];
    mSwap[1].cmd = bufs[1];

    // Check the surface capabilities and formats.
    VkSurfaceCapabilitiesKHR surfCapabilities;
    error = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mGpu, surface, &surfCapabilities);
    LOG_CHECK(!error, "Unable to get surface caps.");

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
    const VkSwapchainCreateInfoKHR swapinfo = {
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
    LOG_CHECK(!error, "Unable to create swap chain.");

    // Extract the VkImage objects from the swap chain.
    LavaVector<VkImage> images;
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &images.size, nullptr);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &images.size, images.alloc());
    LOG_CHECK(images.size > 0, "Unable to get swap chain images.");
    mSwap[0].image = images[0];
    mSwap[1].image = images[1];

    // Create the VkImageView objects.
    VkImageViewCreateInfo viewinfo = {
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
        LOG_CHECK(!error, "Unable to create swap chain image view.");
    }

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
         .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    });
    const VkAttachmentReference colorref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depthref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorref,
    };
    if (createDepthBuffer) {
        subpass.pDepthStencilAttachment = &depthref;
        rpattachments.push_back(VkAttachmentDescription {
            .format = VK_FORMAT_D16_UNORM,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });
    }
    const VkRenderPassCreateInfo rpinfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = rpattachments.size,
        .pAttachments = rpattachments.data,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    error = vkCreateRenderPass(mDevice, &rpinfo, VKALLOC, &mRenderPass);
    LOG_CHECK(!error, "Unable to create render pass.");

    // Create two framebuffers (one for each element in the swap chain)
    LavaVector<VkImageView> fbattachments;
    fbattachments.push_back(mSwap[0].view);
    if (createDepthBuffer) {
        fbattachments.push_back(mDepth.view);
    }
    const VkFramebufferCreateInfo fbinfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mRenderPass,
        .attachmentCount = fbattachments.size,
        .pAttachments = fbattachments.data,
        .width = mExtent.width,
        .height = mExtent.height,
        .layers = 1,
    };
    error = vkCreateFramebuffer(mDevice, &fbinfo, VKALLOC, &mSwap[0].framebuffer);
    LOG_CHECK(!error, "Unable to create framebuffer.");
    fbattachments[0] = mSwap[1].view;
    error = vkCreateFramebuffer(mDevice, &fbinfo, VKALLOC, &mSwap[1].framebuffer);
    LOG_CHECK(!error, "Unable to create framebuffer.");
}

void LavaContextImpl::swap() noexcept {
    std::swap(mSwap[0], mSwap[1]);
}

void LavaContextImpl::initDepthBuffer() noexcept {
    const VkImageCreateInfo image = {
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
    VkMemoryAllocateInfo memalloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    };
    VkResult error = vkCreateImage(mDevice, &image, VKALLOC, &mDepth.image);
    LOG_CHECK(!error, "Unable to create depth image.");

    VkMemoryRequirements memreqs;
    vkGetImageMemoryRequirements(mDevice, mDepth.image, &memreqs);

    memalloc.allocationSize = memreqs.size;
    bool pass = determineMemoryType(memreqs.memoryTypeBits, 0, &memalloc.memoryTypeIndex);
    LOG_CHECK(pass, "Unable to determine memory type.");

    error = vkAllocateMemory(mDevice, &memalloc, VKALLOC, &mDepth.mem);
    LOG_CHECK(!error, "Unable to allocate depth image.");

    error = vkBindImageMemory(mDevice, mDepth.image, mDepth.mem, 0);
    LOG_CHECK(!error, "Unable to bind depth image.");

    // demo_set_image_layout(demo, mDepth.image, VK_IMAGE_ASPECT_DEPTH_BIT,
    //     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);

    const VkImageViewCreateInfo viewinfo = {
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
    LOG_CHECK(!error, "Unable to create depth view.");
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

void LavaContext::initDevice(VkSurfaceKHR surface, bool createDepthBuffer) noexcept {
    upcast(this)->initDevice(surface, createDepthBuffer);
}

void LavaContext::killDevice() noexcept {
    upcast(this)->killDevice();
}

void LavaContext::swap() noexcept {
    upcast(this)->swap();
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

VkCommandPool LavaContext::getCommandPool() const noexcept {
    return upcast(this)->mCommandPool;
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

VkCommandBuffer LavaContext::getCmdBuffer() const noexcept {
    return upcast(this)->mSwap[0].cmd;
}

VkImage LavaContext::getImage() const noexcept {
    return upcast(this)->mSwap[0].image;
}

VkImageView LavaContext::getImageView() const noexcept {
    return upcast(this)->mSwap[0].view;
}

VkFramebuffer LavaContext::getFramebuffer() const noexcept {
    return upcast(this)->mSwap[0].framebuffer;
}

static bool isExtensionSupported(std::string_view ext) noexcept {
    LavaVector<VkExtensionProperties> props;
    vkEnumerateInstanceExtensionProperties(nullptr, &props.size, nullptr);
    VkResult error = vkEnumerateInstanceExtensionProperties(nullptr, &props.size, props.alloc());
    LOG_CHECK(!error, "Unable to enumerate extension properties.");
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
