#include "Core/Base/Marco.h"

#include "VulkanUtil.h"

namespace MiniEngine
{
    VkImageView VulkanUtil::CreateImageView(
        VkDevice                device,
        VkImage&                image,
        VkFormat                format,
        VkImageAspectFlagBits   imageAspectFlagBits,
        VkImageViewType         viewType,
        uint32_t                layoutCount,
        uint32_t                mipLevels)
    {

        VkImageViewCreateInfo imageViewCreateInfo {};
        imageViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image    = image;
        imageViewCreateInfo.viewType = viewType;       // 将图像看作1D纹理、2D纹理、3D纹理或立方体贴图
        imageViewCreateInfo.format   = format;
        imageViewCreateInfo.subresourceRange.aspectMask     = imageAspectFlagBits;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0; // 暂时不考虑Mipmap
        imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0; // 暂时不考虑多图层
        imageViewCreateInfo.subresourceRange.layerCount     = layoutCount;

        VkImageView image_view;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &image_view) != VK_SUCCESS)
            LOG_ERROR("Failed to create image views!");

        return image_view;
    }
}


