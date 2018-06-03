// The MIT License
// Copyright (c) 2018 Philip Rideout

#include <par/LavaLoader.h>
#include <par/LavaTexture.h>
#include <par/LavaLog.h>

#include "LavaInternal.h"

using namespace par;

struct LavaTextureImpl : LavaTexture {
    LavaTextureImpl(Config config) noexcept;
    VkDevice device;
    VmaAllocation stageMem;
    VmaAllocation imageMem;
    VmaAllocator vma;
    VkFormat format;
    VkExtent3D size;
    Properties props;
    VkImageMemoryBarrier barrier1;
    VkBufferImageCopy upload;
    VkImageMemoryBarrier barrier2;        
    void uploadStage(VkCommandBuffer cmd) const noexcept;
};

LAVA_DEFINE_UPCAST(LavaTexture)

LavaTexture* LavaTexture::create(Config config) noexcept {
    return new LavaTextureImpl(config);
}

void LavaTexture::destroy(LavaTexture** that) noexcept {
    LavaTextureImpl& impl = *upcast(*that);
    vmaDestroyBuffer(impl.vma, impl.props.stage, impl.stageMem);
    vmaDestroyImage(impl.vma, impl.props.image, impl.imageMem);
    vkDestroyImageView(impl.device, impl.props.view, VKALLOC);
    delete upcast(&impl);
    *that = nullptr;
}

LavaTextureImpl::LavaTextureImpl(Config config) noexcept : device(config.device) {
    assert(config.device && config.gpu && config.size > 0);
    format = config.info.format;
    size = config.info.extent;
    vma = getVma(config.device, config.gpu);
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = config.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo stageInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vmaCreateBuffer(vma, &bufferInfo, &stageInfo, &props.stage, &stageMem, nullptr);
    if (config.source) {
        void* mappedData;
        vmaMapMemory(vma, stageMem, &mappedData);
        memcpy(mappedData, config.source, config.size);
        vmaUnmapMemory(vma, stageMem);
    }
    VmaAllocationCreateInfo imageInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(vma, &config.info, &imageInfo, &props.image, &imageMem, nullptr);

    VkImageViewCreateInfo colorViewInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = props.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = config.info.format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vkCreateImageView(config.device, &colorViewInfo, VKALLOC, &props.view);

    const int miplevel = 0;

    props.barrier1 = &barrier1;
    barrier1 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = props.image,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = miplevel,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
    };

    props.upload = &upload;
    upload = {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = miplevel,
            .layerCount = 1,
        },
        .imageExtent = {
            .width = config.info.extent.width >> miplevel,
            .height = config.info.extent.height >> miplevel,
            .depth = 1,
        }
    };

    props.barrier2 = &barrier2;
    barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = props.image,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = miplevel,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    };

    // Clients should do:
    //     vkCmdPipelineBarrier(cmdbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, props.barrier1);
    //     vkCmdCopyBufferToImage(cmdbuffer, props.stage, props.image,
    //         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, props.upload);
    //     vkCmdPipelineBarrier(mContext.cmdbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
    //         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, props.barrier2);

}

void LavaTextureImpl::uploadStage(VkCommandBuffer cmd) const noexcept {
    const int miplevel = 0;
    VkImageMemoryBarrier barrier1 {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = props.image,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = miplevel,
            .levelCount = 1,
            .layerCount = 1,
        },
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
    };
    VkBufferImageCopy upload {
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = miplevel,
            .layerCount = 1,
        },
        .imageExtent = {
            .width = size.width >> miplevel,
            .height = size.height >> miplevel,
            .depth = 1,
        }
    };
    VkImageMemoryBarrier barrier2 {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = props.image,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = miplevel,
            .levelCount = 1,
            .layerCount = 1,
        },
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier1);
    vkCmdCopyBufferToImage(cmd, props.stage, props.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &upload);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
}

const LavaTexture::Properties& LavaTexture::getProperties() const noexcept {
    return upcast(this)->props;
}

void LavaTexture::freeStage() noexcept {
    LavaTextureImpl& impl = *upcast(this);
    vmaDestroyBuffer(impl.vma, impl.props.stage, impl.stageMem);
    impl.props.stage = 0;
    impl.stageMem = 0;
}

void LavaTexture::uploadStage(VkCommandBuffer cmd) const noexcept {
    upcast(this)->uploadStage(cmd);
}
