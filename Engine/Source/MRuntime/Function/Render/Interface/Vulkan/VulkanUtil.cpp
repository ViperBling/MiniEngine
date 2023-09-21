#include "VulkanUtil.h"
#include "Core/Base/Marco.h"
#include "vulkan/vulkan_core.h"

namespace MiniEngine
{
    VkImageView VulkanUtil::CreateImageView(
        VkDevice                device,
        VkImage&                image,
        VkFormat                format,
        VkImageAspectFlags      imageAspectFlags,
        VkImageViewType         viewType,
        uint32_t                layoutCount,
        uint32_t                mipLevels)
    {

        VkImageViewCreateInfo imageViewCreateInfo {};
        imageViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image    = image;
        imageViewCreateInfo.viewType = viewType;       // 将图像看作1D纹理、2D纹理、3D纹理或立方体贴图
        imageViewCreateInfo.format   = format;
        imageViewCreateInfo.subresourceRange.aspectMask     = imageAspectFlags;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0; // 暂时不考虑Mipmap
        imageViewCreateInfo.subresourceRange.levelCount     = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0; // 暂时不考虑多图层
        imageViewCreateInfo.subresourceRange.layerCount     = layoutCount;

        VkImageView imageView;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) 
        {
            LOG_ERROR("Failed to create image views!");
            return VK_NULL_HANDLE;
        }

        return imageView;
    }

    VkShaderModule VulkanUtil::CreateShaderModule(VkDevice device, const std::vector<unsigned char> &shaderCode) 
    {
        VkShaderModuleCreateInfo smCI {};
        smCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smCI.codeSize = shaderCode.size();
        smCI.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule(device, &smCI, nullptr, &shaderModule))

        return shaderModule;
    }

    void VulkanUtil::CreateBuffer(
        VkPhysicalDevice physicalDevice, 
        VkDevice device, VkDeviceSize deviceSize, 
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
        VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferCI {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = deviceSize;
        bufferCI.usage = usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device, &bufferCI, nullptr, &buffer))

        VkMemoryRequirements bufferMemoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

        VkMemoryAllocateInfo memAI {};
        memAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAI.allocationSize = bufferMemoryRequirements.size;
        memAI.memoryTypeIndex = VulkanUtil::FindMemoryType(physicalDevice, bufferMemoryRequirements.memoryTypeBits, properties);
        VK_CHECK(vkAllocateMemory(device, &memAI, nullptr, &bufferMemory))

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    uint32_t VulkanUtil::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties physcialDeviceMemProp;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physcialDeviceMemProp);

        for (uint32_t i = 0; i < physcialDeviceMemProp.memoryTypeCount; i++)
        {
            if (typeFilter & (1 << i) && (physcialDeviceMemProp.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }
            
        LOG_ERROR("findMemoryType error");
        return 0;
    }

    std::string ErrorString(VkResult errorCode)
    {
        switch (errorCode)
        {
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
        }
    }
}
