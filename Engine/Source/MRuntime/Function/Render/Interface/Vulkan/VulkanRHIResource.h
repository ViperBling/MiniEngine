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

    class VulkanShader : public RHIShader
    {
    public:
        void SetResource(VkShaderModule res) { mResource = res; }
        VkShaderModule GetResource() const { return mResource; }

    private:
        VkShaderModule mResource;
    };

    class VulkanPipelineLayout : public RHIPipelineLayout {
    public:
        void SetResource(VkPipelineLayout res) { mResource = res; }

        VkPipelineLayout GetResource() const { return mResource; }

    private:
        VkPipelineLayout mResource;
    };
}