#include "VulkanUtil.h"
#include "Core/Base/Marco.h"
#include "vulkan/vulkan_core.h"

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

        VkImageView imageView;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
            LOG_ERROR("Failed to create image views!");
            return VK_NULL_HANDLE;
        }

        return imageView;
    }

    VkShaderModule VulkanUtil::CreateShaderModule(VkDevice device, const std::vector<unsigned char> &shaderCode) {

        VkShaderModuleCreateInfo shaderModuleCreateInfo {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = shaderCode.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            LOG_ERROR("Failed to create shader module!");
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }
}


