#pragma once

#include <iostream>
#include <cassert>
#include <vector>

#include "vulkan/vulkan_core.h"

#define VK_CHECK(f)																				        \
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << MiniEngine::ErrorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace MiniEngine
{
    std::string ErrorString(VkResult errorCode);
    
    class VulkanUtil
    {
    public:
        static VkImageView CreateImageView(
            VkDevice                device,
            VkImage&                image,
            VkFormat                format,
            VkImageAspectFlags      imageAspectFlags,
            VkImageViewType         viewType,
            uint32_t                layoutCount,
            uint32_t                mipLevels
            );

        static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<unsigned char>& shaderCode);
        static void CreateBuffer(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            VkDeviceSize deviceSize,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory
        );
        static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };
}


