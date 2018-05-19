// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>

#include <string>

static void loadLoaderFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*));
static void loadInstanceFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*));
static void loadDeviceFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*));
static PFN_vkVoidFunction vkGetInstanceProcAddrWrapper(void* context, const char* name);
static PFN_vkVoidFunction vkGetDeviceProcAddrWrapper(void* context, const char* name);

static const char* VKLIBRARY_PATH = "libvulkan.1.dylib";

bool LavaLoader::init() {
    const std::string dylibPath = VKLIBRARY_PATH;
    void* module = dlopen(dylibPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        std::cerr << "Unable to load " << dylibPath << std::endl;
        return false;
    }
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(module, "vkGetInstanceProcAddr");
    loadLoaderFunctions(nullptr, vkGetInstanceProcAddrWrapper);
    return true;
}

void LavaLoader::bind(VkInstance instance) {
    loadInstanceFunctions(instance, vkGetInstanceProcAddrWrapper);
    loadDeviceFunctions(instance, vkGetInstanceProcAddrWrapper);
}

static PFN_vkVoidFunction vkGetInstanceProcAddrWrapper(void* context, const char* name) {
    return vkGetInstanceProcAddr((VkInstance) context, name);
}

static PFN_vkVoidFunction vkGetDeviceProcAddrWrapper(void* context, const char* name) {
    return vkGetDeviceProcAddr((VkDevice) context, name);
}

static void loadLoaderFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*)) {
#if defined(VK_VERSION_1_0)
    vkCreateInstance = (PFN_vkCreateInstance) loadcb(context, "vkCreateInstance");
    vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) loadcb(context, "vkEnumerateInstanceExtensionProperties");
    vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) loadcb(context, "vkEnumerateInstanceLayerProperties");
#endif // defined(VK_VERSION_1_0)
#if defined(VK_VERSION_1_1)
    vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion) loadcb(context, "vkEnumerateInstanceVersion");
#endif // defined(VK_VERSION_1_1)
}

static void loadInstanceFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*)) {
#if defined(VK_VERSION_1_0)
    vkCreateDevice = (PFN_vkCreateDevice) loadcb(context, "vkCreateDevice");
    vkDestroyInstance = (PFN_vkDestroyInstance) loadcb(context, "vkDestroyInstance");
    vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) loadcb(context, "vkEnumerateDeviceExtensionProperties");
    vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) loadcb(context, "vkEnumerateDeviceLayerProperties");
    vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) loadcb(context, "vkEnumeratePhysicalDevices");
    vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) loadcb(context, "vkGetDeviceProcAddr");
    vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) loadcb(context, "vkGetPhysicalDeviceFeatures");
    vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) loadcb(context, "vkGetPhysicalDeviceFormatProperties");
    vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) loadcb(context, "vkGetPhysicalDeviceImageFormatProperties");
    vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) loadcb(context, "vkGetPhysicalDeviceMemoryProperties");
    vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) loadcb(context, "vkGetPhysicalDeviceProperties");
    vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) loadcb(context, "vkGetPhysicalDeviceQueueFamilyProperties");
    vkGetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties) loadcb(context, "vkGetPhysicalDeviceSparseImageFormatProperties");
#endif // defined(VK_VERSION_1_0)
#if defined(VK_VERSION_1_1)
    vkEnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups) loadcb(context, "vkEnumeratePhysicalDeviceGroups");
    vkGetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties) loadcb(context, "vkGetPhysicalDeviceExternalBufferProperties");
    vkGetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties) loadcb(context, "vkGetPhysicalDeviceExternalFenceProperties");
    vkGetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties) loadcb(context, "vkGetPhysicalDeviceExternalSemaphoreProperties");
    vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2) loadcb(context, "vkGetPhysicalDeviceFeatures2");
    vkGetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2) loadcb(context, "vkGetPhysicalDeviceFormatProperties2");
    vkGetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2) loadcb(context, "vkGetPhysicalDeviceImageFormatProperties2");
    vkGetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2) loadcb(context, "vkGetPhysicalDeviceMemoryProperties2");
    vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2) loadcb(context, "vkGetPhysicalDeviceProperties2");
    vkGetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2) loadcb(context, "vkGetPhysicalDeviceQueueFamilyProperties2");
    vkGetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2) loadcb(context, "vkGetPhysicalDeviceSparseImageFormatProperties2");
#endif // defined(VK_VERSION_1_1)
#if defined(VK_EXT_acquire_xlib_display)
    vkAcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT) loadcb(context, "vkAcquireXlibDisplayEXT");
    vkGetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT) loadcb(context, "vkGetRandROutputDisplayEXT");
#endif // defined(VK_EXT_acquire_xlib_display)
#if defined(VK_EXT_debug_report)
    vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) loadcb(context, "vkCreateDebugReportCallbackEXT");
    vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT) loadcb(context, "vkDebugReportMessageEXT");
    vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) loadcb(context, "vkDestroyDebugReportCallbackEXT");
#endif // defined(VK_EXT_debug_report)
#if defined(VK_EXT_debug_utils)
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) loadcb(context, "vkCreateDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) loadcb(context, "vkDestroyDebugUtilsMessengerEXT");
    vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT) loadcb(context, "vkSubmitDebugUtilsMessageEXT");
#endif // defined(VK_EXT_debug_utils)
#if defined(VK_EXT_direct_mode_display)
    vkReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT) loadcb(context, "vkReleaseDisplayEXT");
#endif // defined(VK_EXT_direct_mode_display)
#if defined(VK_EXT_display_surface_counter)
    vkGetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT) loadcb(context, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#endif // defined(VK_EXT_display_surface_counter)
#if defined(VK_EXT_sample_locations)
    vkGetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT) loadcb(context, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
#endif // defined(VK_EXT_sample_locations)
#if defined(VK_KHR_android_surface)
    vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) loadcb(context, "vkCreateAndroidSurfaceKHR");
#endif // defined(VK_KHR_android_surface)
#if defined(VK_KHR_device_group_creation)
    vkEnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR) loadcb(context, "vkEnumeratePhysicalDeviceGroupsKHR");
#endif // defined(VK_KHR_device_group_creation)
#if defined(VK_KHR_display)
    vkCreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR) loadcb(context, "vkCreateDisplayModeKHR");
    vkCreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR) loadcb(context, "vkCreateDisplayPlaneSurfaceKHR");
    vkGetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR) loadcb(context, "vkGetDisplayModePropertiesKHR");
    vkGetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR) loadcb(context, "vkGetDisplayPlaneCapabilitiesKHR");
    vkGetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR) loadcb(context, "vkGetDisplayPlaneSupportedDisplaysKHR");
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR) loadcb(context, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
    vkGetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR) loadcb(context, "vkGetPhysicalDeviceDisplayPropertiesKHR");
#endif // defined(VK_KHR_display)
#if defined(VK_KHR_external_fence_capabilities)
    vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR) loadcb(context, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
#endif // defined(VK_KHR_external_fence_capabilities)
#if defined(VK_KHR_external_memory_capabilities)
    vkGetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR) loadcb(context, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
#endif // defined(VK_KHR_external_memory_capabilities)
#if defined(VK_KHR_external_semaphore_capabilities)
    vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR) loadcb(context, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
#endif // defined(VK_KHR_external_semaphore_capabilities)
#if defined(VK_KHR_get_physical_device_properties2)
    vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) loadcb(context, "vkGetPhysicalDeviceFeatures2KHR");
    vkGetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR) loadcb(context, "vkGetPhysicalDeviceFormatProperties2KHR");
    vkGetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR) loadcb(context, "vkGetPhysicalDeviceImageFormatProperties2KHR");
    vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR) loadcb(context, "vkGetPhysicalDeviceMemoryProperties2KHR");
    vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) loadcb(context, "vkGetPhysicalDeviceProperties2KHR");
    vkGetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR) loadcb(context, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    vkGetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR) loadcb(context, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
#endif // defined(VK_KHR_get_physical_device_properties2)
#if defined(VK_KHR_get_surface_capabilities2)
    vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR) loadcb(context, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR) loadcb(context, "vkGetPhysicalDeviceSurfaceFormats2KHR");
#endif // defined(VK_KHR_get_surface_capabilities2)
#if defined(VK_KHR_mir_surface)
    vkCreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR) loadcb(context, "vkCreateMirSurfaceKHR");
    vkGetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR) loadcb(context, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // defined(VK_KHR_mir_surface)
#if defined(VK_KHR_surface)
    vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) loadcb(context, "vkDestroySurfaceKHR");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) loadcb(context, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) loadcb(context, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) loadcb(context, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) loadcb(context, "vkGetPhysicalDeviceSurfaceSupportKHR");
#endif // defined(VK_KHR_surface)
#if defined(VK_KHR_wayland_surface)
    vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) loadcb(context, "vkCreateWaylandSurfaceKHR");
    vkGetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) loadcb(context, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif // defined(VK_KHR_wayland_surface)
#if defined(VK_KHR_win32_surface)
    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) loadcb(context, "vkCreateWin32SurfaceKHR");
    vkGetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) loadcb(context, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // defined(VK_KHR_win32_surface)
#if defined(VK_KHR_xcb_surface)
    vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) loadcb(context, "vkCreateXcbSurfaceKHR");
    vkGetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR) loadcb(context, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // defined(VK_KHR_xcb_surface)
#if defined(VK_KHR_xlib_surface)
    vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) loadcb(context, "vkCreateXlibSurfaceKHR");
    vkGetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR) loadcb(context, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // defined(VK_KHR_xlib_surface)
#if defined(VK_MVK_ios_surface)
    vkCreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK) loadcb(context, "vkCreateIOSSurfaceMVK");
#endif // defined(VK_MVK_ios_surface)
#if defined(VK_MVK_macos_surface)
    vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK) loadcb(context, "vkCreateMacOSSurfaceMVK");
#endif // defined(VK_MVK_macos_surface)
#if defined(VK_NN_vi_surface)
    vkCreateViSurfaceNN = (PFN_vkCreateViSurfaceNN) loadcb(context, "vkCreateViSurfaceNN");
#endif // defined(VK_NN_vi_surface)
#if defined(VK_NVX_device_generated_commands)
    vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX = (PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX) loadcb(context, "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
#endif // defined(VK_NVX_device_generated_commands)
#if defined(VK_NV_external_memory_capabilities)
    vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV) loadcb(context, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif // defined(VK_NV_external_memory_capabilities)
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    vkGetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR) loadcb(context, "vkGetPhysicalDevicePresentRectanglesKHR");
#endif // (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
}

static void loadDeviceFunctions(void* context, PFN_vkVoidFunction (*loadcb)(void*, const char*)) {
#if defined(VK_VERSION_1_0)
    vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers) loadcb(context, "vkAllocateCommandBuffers");
    vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets) loadcb(context, "vkAllocateDescriptorSets");
    vkAllocateMemory = (PFN_vkAllocateMemory) loadcb(context, "vkAllocateMemory");
    vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer) loadcb(context, "vkBeginCommandBuffer");
    vkBindBufferMemory = (PFN_vkBindBufferMemory) loadcb(context, "vkBindBufferMemory");
    vkBindImageMemory = (PFN_vkBindImageMemory) loadcb(context, "vkBindImageMemory");
    vkCmdBeginQuery = (PFN_vkCmdBeginQuery) loadcb(context, "vkCmdBeginQuery");
    vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) loadcb(context, "vkCmdBeginRenderPass");
    vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets) loadcb(context, "vkCmdBindDescriptorSets");
    vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer) loadcb(context, "vkCmdBindIndexBuffer");
    vkCmdBindPipeline = (PFN_vkCmdBindPipeline) loadcb(context, "vkCmdBindPipeline");
    vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers) loadcb(context, "vkCmdBindVertexBuffers");
    vkCmdBlitImage = (PFN_vkCmdBlitImage) loadcb(context, "vkCmdBlitImage");
    vkCmdClearAttachments = (PFN_vkCmdClearAttachments) loadcb(context, "vkCmdClearAttachments");
    vkCmdClearColorImage = (PFN_vkCmdClearColorImage) loadcb(context, "vkCmdClearColorImage");
    vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage) loadcb(context, "vkCmdClearDepthStencilImage");
    vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer) loadcb(context, "vkCmdCopyBuffer");
    vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage) loadcb(context, "vkCmdCopyBufferToImage");
    vkCmdCopyImage = (PFN_vkCmdCopyImage) loadcb(context, "vkCmdCopyImage");
    vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer) loadcb(context, "vkCmdCopyImageToBuffer");
    vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults) loadcb(context, "vkCmdCopyQueryPoolResults");
    vkCmdDispatch = (PFN_vkCmdDispatch) loadcb(context, "vkCmdDispatch");
    vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect) loadcb(context, "vkCmdDispatchIndirect");
    vkCmdDraw = (PFN_vkCmdDraw) loadcb(context, "vkCmdDraw");
    vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed) loadcb(context, "vkCmdDrawIndexed");
    vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect) loadcb(context, "vkCmdDrawIndexedIndirect");
    vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect) loadcb(context, "vkCmdDrawIndirect");
    vkCmdEndQuery = (PFN_vkCmdEndQuery) loadcb(context, "vkCmdEndQuery");
    vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass) loadcb(context, "vkCmdEndRenderPass");
    vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands) loadcb(context, "vkCmdExecuteCommands");
    vkCmdFillBuffer = (PFN_vkCmdFillBuffer) loadcb(context, "vkCmdFillBuffer");
    vkCmdNextSubpass = (PFN_vkCmdNextSubpass) loadcb(context, "vkCmdNextSubpass");
    vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier) loadcb(context, "vkCmdPipelineBarrier");
    vkCmdPushConstants = (PFN_vkCmdPushConstants) loadcb(context, "vkCmdPushConstants");
    vkCmdResetEvent = (PFN_vkCmdResetEvent) loadcb(context, "vkCmdResetEvent");
    vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool) loadcb(context, "vkCmdResetQueryPool");
    vkCmdResolveImage = (PFN_vkCmdResolveImage) loadcb(context, "vkCmdResolveImage");
    vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants) loadcb(context, "vkCmdSetBlendConstants");
    vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias) loadcb(context, "vkCmdSetDepthBias");
    vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds) loadcb(context, "vkCmdSetDepthBounds");
    vkCmdSetEvent = (PFN_vkCmdSetEvent) loadcb(context, "vkCmdSetEvent");
    vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth) loadcb(context, "vkCmdSetLineWidth");
    vkCmdSetScissor = (PFN_vkCmdSetScissor) loadcb(context, "vkCmdSetScissor");
    vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask) loadcb(context, "vkCmdSetStencilCompareMask");
    vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference) loadcb(context, "vkCmdSetStencilReference");
    vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask) loadcb(context, "vkCmdSetStencilWriteMask");
    vkCmdSetViewport = (PFN_vkCmdSetViewport) loadcb(context, "vkCmdSetViewport");
    vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer) loadcb(context, "vkCmdUpdateBuffer");
    vkCmdWaitEvents = (PFN_vkCmdWaitEvents) loadcb(context, "vkCmdWaitEvents");
    vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp) loadcb(context, "vkCmdWriteTimestamp");
    vkCreateBuffer = (PFN_vkCreateBuffer) loadcb(context, "vkCreateBuffer");
    vkCreateBufferView = (PFN_vkCreateBufferView) loadcb(context, "vkCreateBufferView");
    vkCreateCommandPool = (PFN_vkCreateCommandPool) loadcb(context, "vkCreateCommandPool");
    vkCreateComputePipelines = (PFN_vkCreateComputePipelines) loadcb(context, "vkCreateComputePipelines");
    vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool) loadcb(context, "vkCreateDescriptorPool");
    vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) loadcb(context, "vkCreateDescriptorSetLayout");
    vkCreateEvent = (PFN_vkCreateEvent) loadcb(context, "vkCreateEvent");
    vkCreateFence = (PFN_vkCreateFence) loadcb(context, "vkCreateFence");
    vkCreateFramebuffer = (PFN_vkCreateFramebuffer) loadcb(context, "vkCreateFramebuffer");
    vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) loadcb(context, "vkCreateGraphicsPipelines");
    vkCreateImage = (PFN_vkCreateImage) loadcb(context, "vkCreateImage");
    vkCreateImageView = (PFN_vkCreateImageView) loadcb(context, "vkCreateImageView");
    vkCreatePipelineCache = (PFN_vkCreatePipelineCache) loadcb(context, "vkCreatePipelineCache");
    vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout) loadcb(context, "vkCreatePipelineLayout");
    vkCreateQueryPool = (PFN_vkCreateQueryPool) loadcb(context, "vkCreateQueryPool");
    vkCreateRenderPass = (PFN_vkCreateRenderPass) loadcb(context, "vkCreateRenderPass");
    vkCreateSampler = (PFN_vkCreateSampler) loadcb(context, "vkCreateSampler");
    vkCreateSemaphore = (PFN_vkCreateSemaphore) loadcb(context, "vkCreateSemaphore");
    vkCreateShaderModule = (PFN_vkCreateShaderModule) loadcb(context, "vkCreateShaderModule");
    vkDestroyBuffer = (PFN_vkDestroyBuffer) loadcb(context, "vkDestroyBuffer");
    vkDestroyBufferView = (PFN_vkDestroyBufferView) loadcb(context, "vkDestroyBufferView");
    vkDestroyCommandPool = (PFN_vkDestroyCommandPool) loadcb(context, "vkDestroyCommandPool");
    vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool) loadcb(context, "vkDestroyDescriptorPool");
    vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout) loadcb(context, "vkDestroyDescriptorSetLayout");
    vkDestroyDevice = (PFN_vkDestroyDevice) loadcb(context, "vkDestroyDevice");
    vkDestroyEvent = (PFN_vkDestroyEvent) loadcb(context, "vkDestroyEvent");
    vkDestroyFence = (PFN_vkDestroyFence) loadcb(context, "vkDestroyFence");
    vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer) loadcb(context, "vkDestroyFramebuffer");
    vkDestroyImage = (PFN_vkDestroyImage) loadcb(context, "vkDestroyImage");
    vkDestroyImageView = (PFN_vkDestroyImageView) loadcb(context, "vkDestroyImageView");
    vkDestroyPipeline = (PFN_vkDestroyPipeline) loadcb(context, "vkDestroyPipeline");
    vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache) loadcb(context, "vkDestroyPipelineCache");
    vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) loadcb(context, "vkDestroyPipelineLayout");
    vkDestroyQueryPool = (PFN_vkDestroyQueryPool) loadcb(context, "vkDestroyQueryPool");
    vkDestroyRenderPass = (PFN_vkDestroyRenderPass) loadcb(context, "vkDestroyRenderPass");
    vkDestroySampler = (PFN_vkDestroySampler) loadcb(context, "vkDestroySampler");
    vkDestroySemaphore = (PFN_vkDestroySemaphore) loadcb(context, "vkDestroySemaphore");
    vkDestroyShaderModule = (PFN_vkDestroyShaderModule) loadcb(context, "vkDestroyShaderModule");
    vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle) loadcb(context, "vkDeviceWaitIdle");
    vkEndCommandBuffer = (PFN_vkEndCommandBuffer) loadcb(context, "vkEndCommandBuffer");
    vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges) loadcb(context, "vkFlushMappedMemoryRanges");
    vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers) loadcb(context, "vkFreeCommandBuffers");
    vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets) loadcb(context, "vkFreeDescriptorSets");
    vkFreeMemory = (PFN_vkFreeMemory) loadcb(context, "vkFreeMemory");
    vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements) loadcb(context, "vkGetBufferMemoryRequirements");
    vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment) loadcb(context, "vkGetDeviceMemoryCommitment");
    vkGetDeviceQueue = (PFN_vkGetDeviceQueue) loadcb(context, "vkGetDeviceQueue");
    vkGetEventStatus = (PFN_vkGetEventStatus) loadcb(context, "vkGetEventStatus");
    vkGetFenceStatus = (PFN_vkGetFenceStatus) loadcb(context, "vkGetFenceStatus");
    vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements) loadcb(context, "vkGetImageMemoryRequirements");
    vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements) loadcb(context, "vkGetImageSparseMemoryRequirements");
    vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) loadcb(context, "vkGetImageSubresourceLayout");
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) loadcb(context, "vkGetInstanceProcAddr");
    vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData) loadcb(context, "vkGetPipelineCacheData");
    vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults) loadcb(context, "vkGetQueryPoolResults");
    vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity) loadcb(context, "vkGetRenderAreaGranularity");
    vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges) loadcb(context, "vkInvalidateMappedMemoryRanges");
    vkMapMemory = (PFN_vkMapMemory) loadcb(context, "vkMapMemory");
    vkMergePipelineCaches = (PFN_vkMergePipelineCaches) loadcb(context, "vkMergePipelineCaches");
    vkQueueBindSparse = (PFN_vkQueueBindSparse) loadcb(context, "vkQueueBindSparse");
    vkQueueSubmit = (PFN_vkQueueSubmit) loadcb(context, "vkQueueSubmit");
    vkQueueWaitIdle = (PFN_vkQueueWaitIdle) loadcb(context, "vkQueueWaitIdle");
    vkResetCommandBuffer = (PFN_vkResetCommandBuffer) loadcb(context, "vkResetCommandBuffer");
    vkResetCommandPool = (PFN_vkResetCommandPool) loadcb(context, "vkResetCommandPool");
    vkResetDescriptorPool = (PFN_vkResetDescriptorPool) loadcb(context, "vkResetDescriptorPool");
    vkResetEvent = (PFN_vkResetEvent) loadcb(context, "vkResetEvent");
    vkResetFences = (PFN_vkResetFences) loadcb(context, "vkResetFences");
    vkSetEvent = (PFN_vkSetEvent) loadcb(context, "vkSetEvent");
    vkUnmapMemory = (PFN_vkUnmapMemory) loadcb(context, "vkUnmapMemory");
    vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets) loadcb(context, "vkUpdateDescriptorSets");
    vkWaitForFences = (PFN_vkWaitForFences) loadcb(context, "vkWaitForFences");
#endif // defined(VK_VERSION_1_0)
#if defined(VK_VERSION_1_1)
    vkBindBufferMemory2 = (PFN_vkBindBufferMemory2) loadcb(context, "vkBindBufferMemory2");
    vkBindImageMemory2 = (PFN_vkBindImageMemory2) loadcb(context, "vkBindImageMemory2");
    vkCmdDispatchBase = (PFN_vkCmdDispatchBase) loadcb(context, "vkCmdDispatchBase");
    vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask) loadcb(context, "vkCmdSetDeviceMask");
    vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate) loadcb(context, "vkCreateDescriptorUpdateTemplate");
    vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion) loadcb(context, "vkCreateSamplerYcbcrConversion");
    vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate) loadcb(context, "vkDestroyDescriptorUpdateTemplate");
    vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion) loadcb(context, "vkDestroySamplerYcbcrConversion");
    vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2) loadcb(context, "vkGetBufferMemoryRequirements2");
    vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport) loadcb(context, "vkGetDescriptorSetLayoutSupport");
    vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures) loadcb(context, "vkGetDeviceGroupPeerMemoryFeatures");
    vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2) loadcb(context, "vkGetDeviceQueue2");
    vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2) loadcb(context, "vkGetImageMemoryRequirements2");
    vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2) loadcb(context, "vkGetImageSparseMemoryRequirements2");
    vkTrimCommandPool = (PFN_vkTrimCommandPool) loadcb(context, "vkTrimCommandPool");
    vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate) loadcb(context, "vkUpdateDescriptorSetWithTemplate");
#endif // defined(VK_VERSION_1_1)
#if defined(VK_AMD_buffer_marker)
    vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD) loadcb(context, "vkCmdWriteBufferMarkerAMD");
#endif // defined(VK_AMD_buffer_marker)
#if defined(VK_AMD_draw_indirect_count)
    vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD) loadcb(context, "vkCmdDrawIndexedIndirectCountAMD");
    vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD) loadcb(context, "vkCmdDrawIndirectCountAMD");
#endif // defined(VK_AMD_draw_indirect_count)
#if defined(VK_AMD_shader_info)
    vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD) loadcb(context, "vkGetShaderInfoAMD");
#endif // defined(VK_AMD_shader_info)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID) loadcb(context, "vkGetAndroidHardwareBufferPropertiesANDROID");
    vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID) loadcb(context, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
#if defined(VK_ANDROID_native_buffer)
    vkAcquireImageANDROID = (PFN_vkAcquireImageANDROID) loadcb(context, "vkAcquireImageANDROID");
    vkGetSwapchainGrallocUsageANDROID = (PFN_vkGetSwapchainGrallocUsageANDROID) loadcb(context, "vkGetSwapchainGrallocUsageANDROID");
    vkQueueSignalReleaseImageANDROID = (PFN_vkQueueSignalReleaseImageANDROID) loadcb(context, "vkQueueSignalReleaseImageANDROID");
#endif // defined(VK_ANDROID_native_buffer)
#if defined(VK_EXT_debug_marker)
    vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT) loadcb(context, "vkCmdDebugMarkerBeginEXT");
    vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT) loadcb(context, "vkCmdDebugMarkerEndEXT");
    vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT) loadcb(context, "vkCmdDebugMarkerInsertEXT");
    vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT) loadcb(context, "vkDebugMarkerSetObjectNameEXT");
    vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT) loadcb(context, "vkDebugMarkerSetObjectTagEXT");
#endif // defined(VK_EXT_debug_marker)
#if defined(VK_EXT_debug_utils)
    vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT) loadcb(context, "vkCmdBeginDebugUtilsLabelEXT");
    vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT) loadcb(context, "vkCmdEndDebugUtilsLabelEXT");
    vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT) loadcb(context, "vkCmdInsertDebugUtilsLabelEXT");
    vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT) loadcb(context, "vkQueueBeginDebugUtilsLabelEXT");
    vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT) loadcb(context, "vkQueueEndDebugUtilsLabelEXT");
    vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT) loadcb(context, "vkQueueInsertDebugUtilsLabelEXT");
    vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT) loadcb(context, "vkSetDebugUtilsObjectNameEXT");
    vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT) loadcb(context, "vkSetDebugUtilsObjectTagEXT");
#endif // defined(VK_EXT_debug_utils)
#if defined(VK_EXT_discard_rectangles)
    vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT) loadcb(context, "vkCmdSetDiscardRectangleEXT");
#endif // defined(VK_EXT_discard_rectangles)
#if defined(VK_EXT_display_control)
    vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT) loadcb(context, "vkDisplayPowerControlEXT");
    vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT) loadcb(context, "vkGetSwapchainCounterEXT");
    vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT) loadcb(context, "vkRegisterDeviceEventEXT");
    vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT) loadcb(context, "vkRegisterDisplayEventEXT");
#endif // defined(VK_EXT_display_control)
#if defined(VK_EXT_external_memory_host)
    vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT) loadcb(context, "vkGetMemoryHostPointerPropertiesEXT");
#endif // defined(VK_EXT_external_memory_host)
#if defined(VK_EXT_hdr_metadata)
    vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT) loadcb(context, "vkSetHdrMetadataEXT");
#endif // defined(VK_EXT_hdr_metadata)
#if defined(VK_EXT_sample_locations)
    vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT) loadcb(context, "vkCmdSetSampleLocationsEXT");
#endif // defined(VK_EXT_sample_locations)
#if defined(VK_EXT_validation_cache)
    vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT) loadcb(context, "vkCreateValidationCacheEXT");
    vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT) loadcb(context, "vkDestroyValidationCacheEXT");
    vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT) loadcb(context, "vkGetValidationCacheDataEXT");
    vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT) loadcb(context, "vkMergeValidationCachesEXT");
#endif // defined(VK_EXT_validation_cache)
#if defined(VK_GOOGLE_display_timing)
    vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE) loadcb(context, "vkGetPastPresentationTimingGOOGLE");
    vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE) loadcb(context, "vkGetRefreshCycleDurationGOOGLE");
#endif // defined(VK_GOOGLE_display_timing)
#if defined(VK_KHR_bind_memory2)
    vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR) loadcb(context, "vkBindBufferMemory2KHR");
    vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR) loadcb(context, "vkBindImageMemory2KHR");
#endif // defined(VK_KHR_bind_memory2)
#if defined(VK_KHR_descriptor_update_template)
    vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR) loadcb(context, "vkCreateDescriptorUpdateTemplateKHR");
    vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR) loadcb(context, "vkDestroyDescriptorUpdateTemplateKHR");
    vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR) loadcb(context, "vkUpdateDescriptorSetWithTemplateKHR");
#endif // defined(VK_KHR_descriptor_update_template)
#if defined(VK_KHR_device_group)
    vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR) loadcb(context, "vkCmdDispatchBaseKHR");
    vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR) loadcb(context, "vkCmdSetDeviceMaskKHR");
    vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR) loadcb(context, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif // defined(VK_KHR_device_group)
#if defined(VK_KHR_display_swapchain)
    vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR) loadcb(context, "vkCreateSharedSwapchainsKHR");
#endif // defined(VK_KHR_display_swapchain)
#if defined(VK_KHR_external_fence_fd)
    vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR) loadcb(context, "vkGetFenceFdKHR");
    vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR) loadcb(context, "vkImportFenceFdKHR");
#endif // defined(VK_KHR_external_fence_fd)
#if defined(VK_KHR_external_fence_win32)
    vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR) loadcb(context, "vkGetFenceWin32HandleKHR");
    vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR) loadcb(context, "vkImportFenceWin32HandleKHR");
#endif // defined(VK_KHR_external_fence_win32)
#if defined(VK_KHR_external_memory_fd)
    vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR) loadcb(context, "vkGetMemoryFdKHR");
    vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR) loadcb(context, "vkGetMemoryFdPropertiesKHR");
#endif // defined(VK_KHR_external_memory_fd)
#if defined(VK_KHR_external_memory_win32)
    vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR) loadcb(context, "vkGetMemoryWin32HandleKHR");
    vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR) loadcb(context, "vkGetMemoryWin32HandlePropertiesKHR");
#endif // defined(VK_KHR_external_memory_win32)
#if defined(VK_KHR_external_semaphore_fd)
    vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR) loadcb(context, "vkGetSemaphoreFdKHR");
    vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR) loadcb(context, "vkImportSemaphoreFdKHR");
#endif // defined(VK_KHR_external_semaphore_fd)
#if defined(VK_KHR_external_semaphore_win32)
    vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR) loadcb(context, "vkGetSemaphoreWin32HandleKHR");
    vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR) loadcb(context, "vkImportSemaphoreWin32HandleKHR");
#endif // defined(VK_KHR_external_semaphore_win32)
#if defined(VK_KHR_get_memory_requirements2)
    vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR) loadcb(context, "vkGetBufferMemoryRequirements2KHR");
    vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR) loadcb(context, "vkGetImageMemoryRequirements2KHR");
    vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR) loadcb(context, "vkGetImageSparseMemoryRequirements2KHR");
#endif // defined(VK_KHR_get_memory_requirements2)
#if defined(VK_KHR_maintenance1)
    vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR) loadcb(context, "vkTrimCommandPoolKHR");
#endif // defined(VK_KHR_maintenance1)
#if defined(VK_KHR_maintenance3)
    vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR) loadcb(context, "vkGetDescriptorSetLayoutSupportKHR");
#endif // defined(VK_KHR_maintenance3)
#if defined(VK_KHR_push_descriptor)
    vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR) loadcb(context, "vkCmdPushDescriptorSetKHR");
#endif // defined(VK_KHR_push_descriptor)
#if defined(VK_KHR_sampler_ycbcr_conversion)
    vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR) loadcb(context, "vkCreateSamplerYcbcrConversionKHR");
    vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR) loadcb(context, "vkDestroySamplerYcbcrConversionKHR");
#endif // defined(VK_KHR_sampler_ycbcr_conversion)
#if defined(VK_KHR_shared_presentable_image)
    vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR) loadcb(context, "vkGetSwapchainStatusKHR");
#endif // defined(VK_KHR_shared_presentable_image)
#if defined(VK_KHR_swapchain)
    vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) loadcb(context, "vkAcquireNextImageKHR");
    vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) loadcb(context, "vkCreateSwapchainKHR");
    vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) loadcb(context, "vkDestroySwapchainKHR");
    vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) loadcb(context, "vkGetSwapchainImagesKHR");
    vkQueuePresentKHR = (PFN_vkQueuePresentKHR) loadcb(context, "vkQueuePresentKHR");
#endif // defined(VK_KHR_swapchain)
#if defined(VK_NVX_device_generated_commands)
    vkCmdProcessCommandsNVX = (PFN_vkCmdProcessCommandsNVX) loadcb(context, "vkCmdProcessCommandsNVX");
    vkCmdReserveSpaceForCommandsNVX = (PFN_vkCmdReserveSpaceForCommandsNVX) loadcb(context, "vkCmdReserveSpaceForCommandsNVX");
    vkCreateIndirectCommandsLayoutNVX = (PFN_vkCreateIndirectCommandsLayoutNVX) loadcb(context, "vkCreateIndirectCommandsLayoutNVX");
    vkCreateObjectTableNVX = (PFN_vkCreateObjectTableNVX) loadcb(context, "vkCreateObjectTableNVX");
    vkDestroyIndirectCommandsLayoutNVX = (PFN_vkDestroyIndirectCommandsLayoutNVX) loadcb(context, "vkDestroyIndirectCommandsLayoutNVX");
    vkDestroyObjectTableNVX = (PFN_vkDestroyObjectTableNVX) loadcb(context, "vkDestroyObjectTableNVX");
    vkRegisterObjectsNVX = (PFN_vkRegisterObjectsNVX) loadcb(context, "vkRegisterObjectsNVX");
    vkUnregisterObjectsNVX = (PFN_vkUnregisterObjectsNVX) loadcb(context, "vkUnregisterObjectsNVX");
#endif // defined(VK_NVX_device_generated_commands)
#if defined(VK_NV_clip_space_w_scaling)
    vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV) loadcb(context, "vkCmdSetViewportWScalingNV");
#endif // defined(VK_NV_clip_space_w_scaling)
#if defined(VK_NV_external_memory_win32)
    vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV) loadcb(context, "vkGetMemoryWin32HandleNV");
#endif // defined(VK_NV_external_memory_win32)
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
    vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR) loadcb(context, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif // (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR) loadcb(context, "vkGetDeviceGroupPresentCapabilitiesKHR");
    vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR) loadcb(context, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif // (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR) loadcb(context, "vkAcquireNextImage2KHR");
#endif // (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
}

#if defined(VK_VERSION_1_0)
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkBindBufferMemory vkBindBufferMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkCmdBeginQuery vkCmdBeginQuery;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
PFN_vkCmdBlitImage vkCmdBlitImage;
PFN_vkCmdClearAttachments vkCmdClearAttachments;
PFN_vkCmdClearColorImage vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
PFN_vkCmdCopyImage vkCmdCopyImage;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
PFN_vkCmdDispatch vkCmdDispatch;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
PFN_vkCmdEndQuery vkCmdEndQuery;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
PFN_vkCmdFillBuffer vkCmdFillBuffer;
PFN_vkCmdNextSubpass vkCmdNextSubpass;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
PFN_vkCmdPushConstants vkCmdPushConstants;
PFN_vkCmdResetEvent vkCmdResetEvent;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
PFN_vkCmdResolveImage vkCmdResolveImage;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
PFN_vkCmdSetEvent vkCmdSetEvent;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
PFN_vkCmdWaitEvents vkCmdWaitEvents;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
PFN_vkCreateBuffer vkCreateBuffer;
PFN_vkCreateBufferView vkCreateBufferView;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkCreateComputePipelines vkCreateComputePipelines;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkCreateEvent vkCreateEvent;
PFN_vkCreateFence vkCreateFence;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateImage vkCreateImage;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkCreatePipelineCache vkCreatePipelineCache;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkCreateQueryPool vkCreateQueryPool;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkCreateSampler vkCreateSampler;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyBuffer vkDestroyBuffer;
PFN_vkDestroyBufferView vkDestroyBufferView;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
PFN_vkDestroyDevice vkDestroyDevice;
PFN_vkDestroyEvent vkDestroyEvent;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkDestroyQueryPool vkDestroyQueryPool;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkDestroySampler vkDestroySampler;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkGetEventStatus vkGetEventStatus;
PFN_vkGetFenceStatus vkGetFenceStatus;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
PFN_vkMapMemory vkMapMemory;
PFN_vkMergePipelineCaches vkMergePipelineCaches;
PFN_vkQueueBindSparse vkQueueBindSparse;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueueWaitIdle vkQueueWaitIdle;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkResetCommandPool vkResetCommandPool;
PFN_vkResetDescriptorPool vkResetDescriptorPool;
PFN_vkResetEvent vkResetEvent;
PFN_vkResetFences vkResetFences;
PFN_vkSetEvent vkSetEvent;
PFN_vkUnmapMemory vkUnmapMemory;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
PFN_vkWaitForFences vkWaitForFences;
#endif // defined(VK_VERSION_1_0)
#if defined(VK_VERSION_1_1)
PFN_vkBindBufferMemory2 vkBindBufferMemory2;
PFN_vkBindImageMemory2 vkBindImageMemory2;
PFN_vkCmdDispatchBase vkCmdDispatchBase;
PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask;
PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate;
PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion;
PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate;
PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion;
PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups;
PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2;
PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport;
PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures;
PFN_vkGetDeviceQueue2 vkGetDeviceQueue2;
PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2;
PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2;
PFN_vkGetPhysicalDeviceExternalBufferProperties vkGetPhysicalDeviceExternalBufferProperties;
PFN_vkGetPhysicalDeviceExternalFenceProperties vkGetPhysicalDeviceExternalFenceProperties;
PFN_vkGetPhysicalDeviceExternalSemaphoreProperties vkGetPhysicalDeviceExternalSemaphoreProperties;
PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2;
PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2;
PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2;
PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2;
PFN_vkTrimCommandPool vkTrimCommandPool;
PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate;
#endif // defined(VK_VERSION_1_1)
#if defined(VK_AMD_buffer_marker)
PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD;
#endif // defined(VK_AMD_buffer_marker)
#if defined(VK_AMD_draw_indirect_count)
PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD;
PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD;
#endif // defined(VK_AMD_draw_indirect_count)
#if defined(VK_AMD_shader_info)
PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD;
#endif // defined(VK_AMD_shader_info)
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID;
PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID;
#endif // defined(VK_ANDROID_external_memory_android_hardware_buffer)
#if defined(VK_ANDROID_native_buffer)
PFN_vkAcquireImageANDROID vkAcquireImageANDROID;
PFN_vkGetSwapchainGrallocUsageANDROID vkGetSwapchainGrallocUsageANDROID;
PFN_vkQueueSignalReleaseImageANDROID vkQueueSignalReleaseImageANDROID;
#endif // defined(VK_ANDROID_native_buffer)
#if defined(VK_EXT_acquire_xlib_display)
PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT;
PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT;
#endif // defined(VK_EXT_acquire_xlib_display)
#if defined(VK_EXT_debug_marker)
PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
#endif // defined(VK_EXT_debug_marker)
#if defined(VK_EXT_debug_report)
PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
#endif // defined(VK_EXT_debug_report)
#if defined(VK_EXT_debug_utils)
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT;
PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT;
PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT;
PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT;
PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT;
#endif // defined(VK_EXT_debug_utils)
#if defined(VK_EXT_direct_mode_display)
PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT;
#endif // defined(VK_EXT_direct_mode_display)
#if defined(VK_EXT_discard_rectangles)
PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT;
#endif // defined(VK_EXT_discard_rectangles)
#if defined(VK_EXT_display_control)
PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT;
PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT;
PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT;
PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT;
#endif // defined(VK_EXT_display_control)
#if defined(VK_EXT_display_surface_counter)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT;
#endif // defined(VK_EXT_display_surface_counter)
#if defined(VK_EXT_external_memory_host)
PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT;
#endif // defined(VK_EXT_external_memory_host)
#if defined(VK_EXT_hdr_metadata)
PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT;
#endif // defined(VK_EXT_hdr_metadata)
#if defined(VK_EXT_sample_locations)
PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT;
PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT;
#endif // defined(VK_EXT_sample_locations)
#if defined(VK_EXT_validation_cache)
PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT;
PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT;
PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT;
PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT;
#endif // defined(VK_EXT_validation_cache)
#if defined(VK_GOOGLE_display_timing)
PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE;
PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE;
#endif // defined(VK_GOOGLE_display_timing)
#if defined(VK_KHR_android_surface)
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#endif // defined(VK_KHR_android_surface)
#if defined(VK_KHR_bind_memory2)
PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR;
PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR;
#endif // defined(VK_KHR_bind_memory2)
#if defined(VK_KHR_descriptor_update_template)
PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR;
PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR;
PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR;
#endif // defined(VK_KHR_descriptor_update_template)
#if defined(VK_KHR_device_group)
PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR;
PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR;
PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR;
#endif // defined(VK_KHR_device_group)
#if defined(VK_KHR_device_group_creation)
PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR;
#endif // defined(VK_KHR_device_group_creation)
#if defined(VK_KHR_display)
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
#endif // defined(VK_KHR_display)
#if defined(VK_KHR_display_swapchain)
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
#endif // defined(VK_KHR_display_swapchain)
#if defined(VK_KHR_external_fence_capabilities)
PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR;
#endif // defined(VK_KHR_external_fence_capabilities)
#if defined(VK_KHR_external_fence_fd)
PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
#endif // defined(VK_KHR_external_fence_fd)
#if defined(VK_KHR_external_fence_win32)
PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR;
PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR;
#endif // defined(VK_KHR_external_fence_win32)
#if defined(VK_KHR_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR;
#endif // defined(VK_KHR_external_memory_capabilities)
#if defined(VK_KHR_external_memory_fd)
PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
#endif // defined(VK_KHR_external_memory_fd)
#if defined(VK_KHR_external_memory_win32)
PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR;
#endif // defined(VK_KHR_external_memory_win32)
#if defined(VK_KHR_external_semaphore_capabilities)
PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR;
#endif // defined(VK_KHR_external_semaphore_capabilities)
#if defined(VK_KHR_external_semaphore_fd)
PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
#endif // defined(VK_KHR_external_semaphore_fd)
#if defined(VK_KHR_external_semaphore_win32)
PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR;
PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR;
#endif // defined(VK_KHR_external_semaphore_win32)
#if defined(VK_KHR_get_memory_requirements2)
PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR;
PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR;
#endif // defined(VK_KHR_get_memory_requirements2)
#if defined(VK_KHR_get_physical_device_properties2)
PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR;
PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR;
#endif // defined(VK_KHR_get_physical_device_properties2)
#if defined(VK_KHR_get_surface_capabilities2)
PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR;
PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR;
#endif // defined(VK_KHR_get_surface_capabilities2)
#if defined(VK_KHR_maintenance1)
PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR;
#endif // defined(VK_KHR_maintenance1)
#if defined(VK_KHR_maintenance3)
PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR;
#endif // defined(VK_KHR_maintenance3)
#if defined(VK_KHR_mir_surface)
PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR;
PFN_vkGetPhysicalDeviceMirPresentationSupportKHR vkGetPhysicalDeviceMirPresentationSupportKHR;
#endif // defined(VK_KHR_mir_surface)
#if defined(VK_KHR_push_descriptor)
PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR;
#endif // defined(VK_KHR_push_descriptor)
#if defined(VK_KHR_sampler_ycbcr_conversion)
PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR;
PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR;
#endif // defined(VK_KHR_sampler_ycbcr_conversion)
#if defined(VK_KHR_shared_presentable_image)
PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR;
#endif // defined(VK_KHR_shared_presentable_image)
#if defined(VK_KHR_surface)
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
#endif // defined(VK_KHR_surface)
#if defined(VK_KHR_swapchain)
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
#endif // defined(VK_KHR_swapchain)
#if defined(VK_KHR_wayland_surface)
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif // defined(VK_KHR_wayland_surface)
#if defined(VK_KHR_win32_surface)
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif // defined(VK_KHR_win32_surface)
#if defined(VK_KHR_xcb_surface)
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif // defined(VK_KHR_xcb_surface)
#if defined(VK_KHR_xlib_surface)
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif // defined(VK_KHR_xlib_surface)
#if defined(VK_MVK_ios_surface)
PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK;
#endif // defined(VK_MVK_ios_surface)
#if defined(VK_MVK_macos_surface)
PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
#endif // defined(VK_MVK_macos_surface)
#if defined(VK_NN_vi_surface)
PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN;
#endif // defined(VK_NN_vi_surface)
#if defined(VK_NVX_device_generated_commands)
PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX;
PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX;
PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX;
PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX;
PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX;
PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX;
PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX;
PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX;
#endif // defined(VK_NVX_device_generated_commands)
#if defined(VK_NV_clip_space_w_scaling)
PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV;
#endif // defined(VK_NV_clip_space_w_scaling)
#if defined(VK_NV_external_memory_capabilities)
PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif // defined(VK_NV_external_memory_capabilities)
#if defined(VK_NV_external_memory_win32)
PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV;
#endif // defined(VK_NV_external_memory_win32)
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR;
#endif // (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1))
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR;
PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR;
PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR;
#endif // (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR;
#endif // (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
