// This demo uses no uniforms, 1 texture sampler, and 2 vertex attributes:
//   Attributes: Interleaved XYZ (12 bytes) and UV (8 bytes)
//   Textures:   2x2 VK__B8G8R8A8_UNORM

/*
 * Draw a textured triangle with depth testing.  This is written against Intel
 * ICD.  It does not do state transition nor object memory binding like it
 * should.  It also does no error checking.
 */

#include <vector>
#include <string>

#include <par/LavaCompiler.h>
#include <par/LavaContext.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>

#include <GLFW/glfw3.h>

#define DEMO_TEXTURE_COUNT 1
#define VERTEX_BUFFER_BIND_ID 0
#define DEMO_WIDTH 640
#define DEMO_HEIGHT 480

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ERR_EXIT(err_msg, err_class)                                           \
    do {                                                                       \
        printf(err_msg);                                                       \
        fflush(stdout);                                                        \
        exit(1);                                                               \
    } while (0)

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
    {                                                                          \
        demo.fp##entrypoint =                                                 \
            (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);    \
        if (demo.fp##entrypoint == NULL) {                                    \
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint,      \
                     "vkGetDeviceProcAddr Failure");                           \
        }                                                                      \
    }

using namespace par;

LavaCompiler* gLavaCompiler;
LavaContext* gLavaContext;

static const std::string vertShaderGLSL = R"GLSL(
#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;
layout(location=0) out vec2 TexCoord;
void main() {
    gl_Position = position;
    TexCoord = uv;
}
)GLSL";

static const std::string fragShaderGLSL = R"GLSL(
#version 450
layout(location=0) out vec4 Color;
layout(location=0) in vec2 uv;
layout(binding=0, set=0) uniform sampler2D tex;
void main() {
    Color = texture(tex, uv);
}
)GLSL";

struct texture_object {
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory mem;
    VkImageView view;
    uint32_t tex_width, tex_height;
};

struct Demo {
    GLFWwindow* window;
    VkSurfaceKHR surface;
    bool use_staging_buffer;

    // struct {
    //     VkFormat format;
    //     VkImage image;
    //     VkDeviceMemory mem;
    //     VkImageView view;
    // } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkPipelineVertexInputStateCreateInfo vi;
        VkVertexInputBindingDescription vi_bindings[1];
        VkVertexInputAttributeDescription vi_attrs[2];
    } vertices;

    VkCommandBuffer setup_cmd; // Command Buffer for initialization commands

    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkPipeline pipeline;
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    int32_t curFrame;
    int32_t frameCount;

    float depthStencil;
    float depthIncrement;

    uint32_t current_buffer;
    uint32_t queue_count;

    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
};

static bool memory_type_from_properties(Demo *demo, uint32_t typeBits,
                                        VkFlags requirements_mask,
                                        uint32_t *typeIndex) {
    uint32_t i;
    // Search memtypes to find first index with those properties
    for (i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((gLavaContext->getMemoryProperties().memoryTypes[i].propertyFlags &
                 requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

static void demo_flush_init_cmd(Demo *demo) {
    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->setup_cmd);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {demo->setup_cmd};
    VkFence nullFence = {VK_NULL_HANDLE};
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
                                .waitSemaphoreCount = 0,
                                .pWaitSemaphores = NULL,
                                .pWaitDstStageMask = NULL,
                                .commandBufferCount = 1,
                                .pCommandBuffers = cmd_bufs,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(gLavaContext->getQueue(), 1, &submit_info, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(gLavaContext->getQueue());
    assert(!err);

    vkFreeCommandBuffers(gLavaContext->getDevice(), gLavaContext->getCommandPool(), 1, cmd_bufs);
    demo->setup_cmd = VK_NULL_HANDLE;
}

static void demo_set_image_layout(Demo *demo, VkImage image, VkImageAspectFlagBits aspectMask,
        VkImageLayout old_image_layout, VkImageLayout new_image_layout,
        VkAccessFlags srcAccessMask) {

    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE) {
        const VkCommandBufferAllocateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = NULL,
            .commandPool = gLavaContext->getCommandPool(),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        err = vkAllocateCommandBuffers(gLavaContext->getDevice(), &cmd, &demo->setup_cmd);
        assert(!err);

        VkCommandBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = 0,
            .pInheritanceInfo = NULL,
        };
        err = vkBeginCommandBuffer(demo->setup_cmd, &cmd_buf_info);
        assert(!err);
    }

    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = {aspectMask, 0, 1, 0, 1}};

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(demo->setup_cmd, src_stages, dest_stages, 0, 0, NULL,
                         0, NULL, 1, pmemory_barrier);
}

static void demo_draw_build_cmd(Demo *demo) {
    const VkCommandBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    const VkClearValue clear_values[2] = {
            [0] = {.color.float32 = {0.7f, 0.2f, 0.2f, 0.2f}},
            [1] = {.depthStencil = {demo->depthStencil, 0}},
    };
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = gLavaContext->getRenderPass(),
        .framebuffer = gLavaContext->getFramebuffer(),
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent = gLavaContext->getSize(),
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkBeginCommandBuffer(gLavaContext->getCmdBuffer(), &cmd_buf_info);
    assert(!err);

    // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care what
    // happens to the previous contents of the image
    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = gLavaContext->getImage(),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    vkCmdPipelineBarrier(gLavaContext->getCmdBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, &image_memory_barrier);
    vkCmdBeginRenderPass(gLavaContext->getCmdBuffer(), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(gLavaContext->getCmdBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                      demo->pipeline);
    vkCmdBindDescriptorSets(gLavaContext->getCmdBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
                            demo->pipeline_layout, 0, 1, &demo->desc_set, 0,
                            NULL);

    VkViewport viewport = {};
    viewport.width = gLavaContext->getSize().width;
    viewport.height = gLavaContext->getSize().height;
    vkCmdSetViewport(gLavaContext->getCmdBuffer(), 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent = gLavaContext->getSize();
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(gLavaContext->getCmdBuffer(), 0, 1, &scissor);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(gLavaContext->getCmdBuffer(), VERTEX_BUFFER_BIND_ID, 1,
                           &demo->vertices.buf, offsets);

    vkCmdDraw(gLavaContext->getCmdBuffer(), 3, 1, 0, 0);
    vkCmdEndRenderPass(gLavaContext->getCmdBuffer());

    VkImageMemoryBarrier prePresentBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    prePresentBarrier.image = gLavaContext->getImage();
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(gLavaContext->getCmdBuffer(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, pmemory_barrier);

    err = vkEndCommandBuffer(gLavaContext->getCmdBuffer());
    assert(!err);
}

static void demo_draw(Demo *demo) {
    VkResult U_ASSERT_ONLY err;
    VkSemaphore imageAcquiredSemaphore, drawCompleteSemaphore;
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };

    err = vkCreateSemaphore(gLavaContext->getDevice(), &semaphoreCreateInfo,
                            NULL, &imageAcquiredSemaphore);
    assert(!err);

    err = vkCreateSemaphore(gLavaContext->getDevice(), &semaphoreCreateInfo,
                            NULL, &drawCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    gLavaContext->swap();
    err = demo->fpAcquireNextImageKHR(gLavaContext->getDevice(), gLavaContext->getSwapchain(),
            UINT64_MAX, imageAcquiredSemaphore, (VkFence)0, &demo->current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        printf("Out of date.\n");
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    demo_flush_init_cmd(demo);

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    demo_draw_build_cmd(demo);
    VkFence nullFence = VK_NULL_HANDLE;
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkCommandBuffer cmdbufs[] = { gLavaContext->getCmdBuffer() };
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAcquiredSemaphore,
        .pWaitDstStageMask = &pipe_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = cmdbufs,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &drawCompleteSemaphore
    };

    err = vkQueueSubmit(gLavaContext->getQueue(), 1, &submit_info, nullFence);
    assert(!err);

    VkSwapchainKHR chains[] = { gLavaContext->getSwapchain() };
    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &drawCompleteSemaphore,
        .swapchainCount = 1,
        .pSwapchains = chains,
        .pImageIndices = &demo->current_buffer,
    };

    err = demo->fpQueuePresentKHR(gLavaContext->getQueue(), &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        printf("Out of date\n");
    } else if (err == VK_SUBOPTIMAL_KHR) {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    err = vkQueueWaitIdle(gLavaContext->getQueue());
    assert(err == VK_SUCCESS);

    vkDestroySemaphore(gLavaContext->getDevice(), imageAcquiredSemaphore, NULL);
    vkDestroySemaphore(gLavaContext->getDevice(), drawCompleteSemaphore, NULL);
}

static void
demo_prepare_texture_image(Demo *demo, const uint32_t *tex_colors,
                           struct texture_object *tex_obj, VkImageTiling tiling,
                           VkImageUsageFlags usage, VkFlags required_props) {
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 2;
    const int32_t tex_height = 2;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tex_format,
        .extent = {tex_width, tex_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
    };
    VkMemoryAllocateInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };

    VkMemoryRequirements mem_reqs;

    err =
        vkCreateImage(gLavaContext->getDevice(), &image_create_info, NULL, &tex_obj->image);
    assert(!err);

    vkGetImageMemoryRequirements(gLavaContext->getDevice(), tex_obj->image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    pass =
        memory_type_from_properties(demo, mem_reqs.memoryTypeBits,
                                    required_props, &mem_alloc.memoryTypeIndex);
    assert(pass);

    /* allocate memory */
    err = vkAllocateMemory(gLavaContext->getDevice(), &mem_alloc, NULL, &tex_obj->mem);
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(gLavaContext->getDevice(), tex_obj->image, tex_obj->mem, 0);
    assert(!err);

    if (required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void *data;
        int32_t x, y;

        vkGetImageSubresourceLayout(gLavaContext->getDevice(), tex_obj->image, &subres,
                                    &layout);

        err = vkMapMemory(gLavaContext->getDevice(), tex_obj->mem, 0,
                          mem_alloc.allocationSize, 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *)((char *)data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[(x & 1) ^ (y & 1)];
        }

        vkUnmapMemory(gLavaContext->getDevice(), tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(demo, tex_obj->image, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_LAYOUT_PREINITIALIZED, tex_obj->imageLayout,
                          VkAccessFlags(VK_ACCESS_HOST_WRITE_BIT));
    /* setting the image layout does not reference the actual memory so no need
     * to add a mem ref */
}

static void demo_destroy_texture_image(Demo *demo, struct texture_object *tex_obj) {
    vkDestroyImage(gLavaContext->getDevice(), tex_obj->image, NULL);
    vkFreeMemory(gLavaContext->getDevice(), tex_obj->mem, NULL);
}

static void demo_prepare_textures(Demo *demo) {
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormatProperties props;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        {0xff7f8050, 0xff008080},
    };
    uint32_t i;
    VkResult U_ASSERT_ONLY err;

    vkGetPhysicalDeviceFormatProperties(gLavaContext->getGpu(), tex_format, &props);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        if ((props.linearTilingFeatures &
             VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
            !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(
                demo, tex_colors[i], &demo->textures[i], VK_IMAGE_TILING_LINEAR,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        } else if (props.optimalTilingFeatures &
                   VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(
                demo, tex_colors[i], &staging_texture, VK_IMAGE_TILING_LINEAR,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            demo_prepare_texture_image(
                demo, tex_colors[i], &demo->textures[i],
                VK_IMAGE_TILING_OPTIMAL,
                (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            demo_set_image_layout(demo, staging_texture.image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  staging_texture.imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  0);

            demo_set_image_layout(demo, demo->textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  demo->textures[i].imageLayout,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  0);

            VkImageCopy copy_region = {
                .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .srcOffset = {0, 0, 0},
                .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .dstOffset = {0, 0, 0},
                .extent = {staging_texture.tex_width,
                           staging_texture.tex_height, 1},
            };
            vkCmdCopyImage(
                demo->setup_cmd, staging_texture.image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, demo->textures[i].image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            demo_set_image_layout(demo, demo->textures[i].image,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  demo->textures[i].imageLayout,
                                  0);

            demo_flush_init_cmd(demo);

            demo_destroy_texture_image(demo, &staging_texture);
        } else {
            /* Can't support VK_FORMAT_B8G8R8A8_UNORM !? */
            assert(!"No support for B8G8R8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };
        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .components =
                {
                 VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                 VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A,
                },
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            .flags = 0,
        };

        /* create sampler */
        err = vkCreateSampler(gLavaContext->getDevice(), &sampler, NULL,
                              &demo->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = demo->textures[i].image;
        err = vkCreateImageView(gLavaContext->getDevice(), &view, NULL,
                                &demo->textures[i].view);
        assert(!err);
    }
}

static void demo_prepare_vertices(Demo *demo) {
    // clang-format off
    const float vb[3][5] = {
        /*      position             texcoord */
        { -1.0f, -1.0f,  0.25f,     0.0f, 0.0f },
        {  1.0f, -1.0f,  0.25f,     1.0f, 0.0f },
        {  0.0f,  1.0f,  1.0f,      0.5f, 1.0f },
    };
    // clang-format on
    const VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = sizeof(vb),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .flags = 0,
    };
    VkMemoryAllocateInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };
    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    bool U_ASSERT_ONLY pass;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = vkCreateBuffer(gLavaContext->getDevice(), &buf_info, NULL, &demo->vertices.buf);
    assert(!err);

    vkGetBufferMemoryRequirements(gLavaContext->getDevice(), demo->vertices.buf, &mem_reqs);
    assert(!err);

    mem_alloc.allocationSize = mem_reqs.size;
    pass = memory_type_from_properties(demo, mem_reqs.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &mem_alloc.memoryTypeIndex);
    assert(pass);

    err = vkAllocateMemory(gLavaContext->getDevice(), &mem_alloc, NULL, &demo->vertices.mem);
    assert(!err);

    err = vkMapMemory(gLavaContext->getDevice(), demo->vertices.mem, 0,
                      mem_alloc.allocationSize, 0, &data);
    assert(!err);

    memcpy(data, vb, sizeof(vb));

    vkUnmapMemory(gLavaContext->getDevice(), demo->vertices.mem);

    err = vkBindBufferMemory(gLavaContext->getDevice(), demo->vertices.buf,
                             demo->vertices.mem, 0);
    assert(!err);

    demo->vertices.vi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.vertexBindingDescriptionCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.vertexAttributeDescriptionCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_bindings[0].stride = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[0].location = 0;
    demo->vertices.vi_attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    demo->vertices.vi_attrs[0].offset = 0;

    demo->vertices.vi_attrs[1].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[1].location = 1;
    demo->vertices.vi_attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    demo->vertices.vi_attrs[1].offset = sizeof(float) * 3;
}

static void demo_prepare_descriptor_layout(Demo *demo) {
    const VkDescriptorSetLayoutBinding layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = DEMO_TEXTURE_COUNT,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(gLavaContext->getDevice(), &descriptor_layout, NULL,
                                      &demo->desc_layout);
    assert(!err);

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .setLayoutCount = 1,
        .pSetLayouts = &demo->desc_layout,
    };

    err = vkCreatePipelineLayout(gLavaContext->getDevice(), &pPipelineLayoutCreateInfo, NULL,
                                 &demo->pipeline_layout);
    assert(!err);
}

static VkShaderModule
demo_prepare_shader_module(Demo *demo, const uint32_t *code, size_t nwords) {
    VkShaderModule module;
    VkResult U_ASSERT_ONLY err;
    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.codeSize = nwords * 4;
    moduleCreateInfo.pCode = code;
    moduleCreateInfo.flags = 0;
    err = vkCreateShaderModule(gLavaContext->getDevice(), &moduleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

static VkShaderModule demo_prepare_vs(Demo *demo) {
    std::vector<uint32_t> compiled;
    bool success = gLavaCompiler->compile(LavaCompiler::VERTEX, vertShaderGLSL, &compiled);
    assert(success);
    demo->vert_shader_module = demo_prepare_shader_module(demo, compiled.data(), compiled.size());
    return demo->vert_shader_module;
}

static VkShaderModule demo_prepare_fs(Demo *demo) {
    std::vector<uint32_t> compiled;
    bool success = gLavaCompiler->compile(LavaCompiler::FRAGMENT, fragShaderGLSL, &compiled);
    assert(success);
    demo->frag_shader_module = demo_prepare_shader_module(demo, compiled.data(), compiled.size());
    return demo->frag_shader_module;
}

static void demo_prepare_pipeline(Demo *demo) {
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;

    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;

    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    gLavaCompiler = LavaCompiler::create();

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo_prepare_vs(demo);
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo_prepare_fs(demo);
    shaderStages[1].pName = "main";

    LavaCompiler::destroy(&gLavaCompiler);

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.pStages = shaderStages;
    pipeline.renderPass = gLavaContext->getRenderPass();
    pipeline.pDynamicState = &dynamicState;

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(gLavaContext->getDevice(), &pipelineCache, NULL,
                                &demo->pipelineCache);
    assert(!err);
    err = vkCreateGraphicsPipelines(gLavaContext->getDevice(), demo->pipelineCache, 1,
                                    &pipeline, NULL, &demo->pipeline);
    assert(!err);

    vkDestroyPipelineCache(gLavaContext->getDevice(), demo->pipelineCache, NULL);

    vkDestroyShaderModule(gLavaContext->getDevice(), demo->frag_shader_module, NULL);
    vkDestroyShaderModule(gLavaContext->getDevice(), demo->vert_shader_module, NULL);
}

static void demo_prepare_descriptor_pool(Demo *demo) {
    const VkDescriptorPoolSize type_count = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = DEMO_TEXTURE_COUNT,
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &type_count,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(gLavaContext->getDevice(), &descriptor_pool, NULL,
                                 &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(Demo *demo) {
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet write;
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = demo->desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &demo->desc_layout};
    err = vkAllocateDescriptorSets(gLavaContext->getDevice(), &alloc_info, &demo->desc_set);
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&write, 0, sizeof(write));
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = demo->desc_set;
    write.descriptorCount = DEMO_TEXTURE_COUNT;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = tex_descs;

    vkUpdateDescriptorSets(gLavaContext->getDevice(), 1, &write, 0, NULL);
}

static void demo_prepare(Demo *demo) {
    demo_prepare_textures(demo);
    demo_prepare_vertices(demo);
    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);
}

static void demo_error_callback(int error, const char* description) {
    printf("GLFW error: %s\n", description);
    fflush(stdout);
}

static void demo_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void demo_refresh_callback(GLFWwindow* window) {
    demo_draw(static_cast<Demo*>(glfwGetWindowUserPointer(window)));
}

static void demo_run(Demo *demo) {
    while (!glfwWindowShouldClose(demo->window)) {
        glfwPollEvents();

        demo_draw(demo);

        if (demo->depthStencil > 0.99f)
            demo->depthIncrement = -0.001f;
        if (demo->depthStencil < 0.8f)
            demo->depthIncrement = 0.001f;

        demo->depthStencil += demo->depthIncrement;

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(gLavaContext->getDevice());
        demo->curFrame++;
        if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount)
            glfwSetWindowShouldClose(demo->window, GLFW_TRUE);
    }
}

static void demo_cleanup(Demo *demo) {

    vkDestroyDescriptorPool(gLavaContext->getDevice(), demo->desc_pool, NULL);

    if (demo->setup_cmd) {
        vkFreeCommandBuffers(gLavaContext->getDevice(), gLavaContext->getCommandPool(), 1,
                &demo->setup_cmd);
    }

    vkDestroyPipeline(gLavaContext->getDevice(), demo->pipeline, NULL);
    vkDestroyPipelineLayout(gLavaContext->getDevice(), demo->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(gLavaContext->getDevice(), demo->desc_layout, NULL);

    vkDestroyBuffer(gLavaContext->getDevice(), demo->vertices.buf, NULL);
    vkFreeMemory(gLavaContext->getDevice(), demo->vertices.mem, NULL);

    for (uint32_t i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(gLavaContext->getDevice(), demo->textures[i].view, NULL);
        vkDestroyImage(gLavaContext->getDevice(), demo->textures[i].image, NULL);
        vkFreeMemory(gLavaContext->getDevice(), demo->textures[i].mem, NULL);
        vkDestroySampler(gLavaContext->getDevice(), demo->textures[i].sampler, NULL);
    }

    // vkDestroyImageView(gLavaContext->getDevice(), demo->depth.view, NULL);
    // vkDestroyImage(gLavaContext->getDevice(), demo->depth.image, NULL);
    // vkFreeMemory(gLavaContext->getDevice(), demo->depth.mem, NULL);

    gLavaContext->killDevice();

    vkDestroySurfaceKHR(gLavaContext->getInstance(), demo->surface, NULL);

    LavaContext::destroy(&gLavaContext);

    glfwDestroyWindow(demo->window);
    glfwTerminate();
}

int main(const int argc, const char *argv[]) {
    Demo demo = {
        .frameCount = INT32_MAX,
        .depthStencil = 1.0,
        .depthIncrement = -0.01f,
    };

    glfwSetErrorCallback(demo_error_callback);

    if (!glfwInit()) {
        printf("Cannot initialize GLFW.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    if (!glfwVulkanSupported()) {
        printf("GLFW failed to find the Vulkan loader.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    float xscale = 0, yscale = 0;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
    demo.window = glfwCreateWindow(DEMO_WIDTH / xscale, DEMO_HEIGHT / yscale, "hellolava",
            NULL, NULL);
    if (!demo.window) {
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }

    glfwSetWindowUserPointer(demo.window, &demo);
    glfwSetWindowRefreshCallback(demo.window, demo_refresh_callback);
    glfwSetKeyCallback(demo.window, demo_key_callback);

    gLavaContext = LavaContext::create(true);

    glfwCreateWindowSurface(gLavaContext->getInstance(), demo.window, NULL, &demo.surface);

    gLavaContext->initDevice(demo.surface, true);

    GET_DEVICE_PROC_ADDR(gLavaContext->getDevice(), AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(gLavaContext->getDevice(), QueuePresentKHR);

    demo_prepare(&demo);
    demo_run(&demo);
    printf("Shutting down...\n");
    fflush(stdout);
    demo_cleanup(&demo);

    return 0;
}
