#pragma once

#include "Function/Render/Interface/RHIStruct.h"
#include "vulkan/vulkan_core.h"

namespace MiniEngine
{
    class VulkanQueue : public RHIQueue
    {
    public:
        void SetResource(VkQueue resource) { mResource = resource; }
        VkQueue GetResource() const { return mResource; }

    private:
        VkQueue mResource;
    };

    class VulkanImageView : public RHIImageView
    {
    public:
        void SetResource(VkImageView resource) { mResource = resource; }
        VkImageView GetResource() const { return mResource; }

    private:
        VkImageView mResource;
    };
}