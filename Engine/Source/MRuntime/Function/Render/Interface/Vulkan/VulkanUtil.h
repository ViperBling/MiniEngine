#pragma once

#include <vector>

#include "vulkan/vulkan_core.h"

namespace MiniEngine
{
    class VulkanUtil
    {
    public:
        static VkImageView CreateImageView(
            VkDevice                device,
            VkImage&                image,
            VkFormat                format,
            VkImageAspectFlagBits   imageAspectFlagBits,
            VkImageViewType         viewType,
            uint32_t                layoutCount,
            uint32_t                mipLevels
            );

        static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<unsigned char>& shaderCode);
    };
}


