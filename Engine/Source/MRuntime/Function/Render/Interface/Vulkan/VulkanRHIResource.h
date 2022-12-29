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

    class VulkanPipelineLayout : public RHIPipelineLayout
    {
    public:
        void SetResource(VkPipelineLayout res) { mResource = res; }

        VkPipelineLayout GetResource() const { return mResource; }

    private:
        VkPipelineLayout mResource;
    };

    class VulkanRenderPass : public RHIRenderPass
    {
    public:
        void SetResource(VkRenderPass res) { mResource = res; }
        VkRenderPass GetResource() const { return mResource; }

    private:
        VkRenderPass mResource;
    };

    class VulkanPipeline : public RHIPipeline
    {
    public:
        void SetResource(VkPipeline res) { mResource = res; }
        VkPipeline GetResource() const { return mResource; }

    private:
        VkPipeline mResource;
    };

    class VulkanPipelineCache : public RHIPipelineCache
    {
    public:
        void SetResource(VkPipelineCache res) { mResource = res; }
        VkPipelineCache GetResource() const { return mResource; }

    private:
        VkPipelineCache mResource;
    };

    class VulkanFrameBuffer : public RHIFrameBuffer
    {
    public:
        void SetResource(VkFramebuffer res) { mResource = res; }
        VkFramebuffer GetResource() const { return mResource; }

    private:
        VkFramebuffer mResource;
    };

    class VulkanCommandPool : public RHICommandPool
    {
    public:
        void SetResource(VkCommandPool res) { mResource = res; }
        VkCommandPool GetResource() const { return mResource; }

    private:
        VkCommandPool mResource;
    };

    class VulkanCommandBuffer : public RHICommandBuffer
    {
    public:
        void SetResource(VkCommandBuffer res) { mResource = res; }
        VkCommandBuffer GetResource() const { return mResource; }

    private:
        VkCommandBuffer mResource;
    };
}