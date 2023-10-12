#pragma once

#include "Function/Render/Interface/RHIStruct.hpp"
#include "vulkan/vulkan_core.h"

namespace MiniEngine
{
    class VulkanBuffer : public RHIBuffer
    {
    public:
        void SetResource(VkBuffer res)
        {
            m_resource = res;
        }
        VkBuffer GetResource() const
        {
            return m_resource;
        }
    private:
        VkBuffer m_resource;
    };
    class VulkanBufferView : public RHIBufferView
    {
    public:
        void SetResource(VkBufferView res)
        {
            m_resource = res;
        }
        VkBufferView GetResource() const
        {
            return m_resource;
        }
    private:
        VkBufferView m_resource;
    };
    class VulkanCommandBuffer : public RHICommandBuffer
    {
    public:
        void SetResource(VkCommandBuffer res)
        {
            m_resource = res;
        }
        const VkCommandBuffer GetResource() const
        {
            return m_resource;
        }
    private:
        VkCommandBuffer m_resource;
    };
    class VulkanCommandPool : public RHICommandPool
    {
    public:
        void SetResource(VkCommandPool res)
        {
            m_resource = res;
        }
        VkCommandPool GetResource() const
        {
            return m_resource;
        }
    private:
        VkCommandPool m_resource;
    };
    class VulkanDescriptorPool : public RHIDescriptorPool
    {
    public:
        void SetResource(VkDescriptorPool res)
        {
            m_resource = res;
        }
        VkDescriptorPool GetResource() const
        {
            return m_resource;
        }
    private:
        VkDescriptorPool m_resource;
    };
    class VulkanDescriptorSet : public RHIDescriptorSet
    {
    public:
        void SetResource(VkDescriptorSet res)
        {
            m_resource = res;
        }
        VkDescriptorSet GetResource() const
        {
            return m_resource;
        }
    private:
        VkDescriptorSet m_resource;
    };
    class VulkanDescriptorSetLayout : public RHIDescriptorSetLayout
    {
    public:
        void SetResource(VkDescriptorSetLayout res)
        {
            m_resource = res;
        }
        VkDescriptorSetLayout GetResource() const
        {
            return m_resource;
        }
    private:
        VkDescriptorSetLayout m_resource;
    };
    class VulkanDevice : public RHIDevice
    {
    public:
        void SetResource(VkDevice res)
        {
            m_resource = res;
        }
        VkDevice GetResource() const
        {
            return m_resource;
        }
    private:
        VkDevice m_resource;
    };
    class VulkanDeviceMemory : public RHIDeviceMemory
    {
    public:
        void SetResource(VkDeviceMemory res)
        {
            m_resource = res;
        }
        VkDeviceMemory GetResource() const
        {
            return m_resource;
        }
    private:
        VkDeviceMemory m_resource;
    };
    class VulkanEvent : public RHIEvent
    {
    public:
        void SetResource(VkEvent res)
        {
            m_resource = res;
        }
        VkEvent GetResource() const
        {
            return m_resource;
        }
    private:
        VkEvent m_resource;
    };
    class VulkanFence : public RHIFence
    {
    public:
        void SetResource(VkFence res)
        {
            m_resource = res;
        }
        VkFence GetResource() const
        {
            return m_resource;
        }
    private:
        VkFence m_resource;
    };
    class VulkanFrameBuffer : public RHIFrameBuffer
    {
    public:
        void SetResource(VkFramebuffer res)
        {
            m_resource = res;
        }
        VkFramebuffer GetResource() const
        {
            return m_resource;
        }
    private:
        VkFramebuffer m_resource;
    };
    class VulkanImage : public RHIImage
    {
    public:
        void SetResource(VkImage res)
        {
            m_resource = res;
        }
        VkImage &GetResource()
        {
            return m_resource;
        }
    private:
        VkImage m_resource;
    };
    class VulkanImageView : public RHIImageView
    {
    public:
        void SetResource(VkImageView res)
        {
            m_resource = res;
        }
        VkImageView GetResource() const
        {
            return m_resource;
        }
    private:
        VkImageView m_resource;
    };
    class VulkanInstance : public RHIInstance
    {
    public:
        void SetResource(VkInstance res)
        {
            m_resource = res;
        }
        VkInstance GetResource() const
        {
            return m_resource;
        }
    private:
        VkInstance m_resource;
    };
    class VulkanQueue : public RHIQueue
    {
    public:
        void SetResource(VkQueue res)
        {
            m_resource = res;
        }
        VkQueue GetResource() const
        {
            return m_resource;
        }
    private:
        VkQueue m_resource;
    };
    class VulkanPhysicalDevice : public RHIPhysicalDevice
    {
    public:
        void SetResource(VkPhysicalDevice res)
        {
            m_resource = res;
        }
        VkPhysicalDevice GetResource() const
        {
            return m_resource;
        }
    private:
        VkPhysicalDevice m_resource;
    };
    class VulkanPipeline : public RHIPipeline
    {
    public:
        void SetResource(VkPipeline res)
        {
            m_resource = res;
        }
        VkPipeline GetResource() const
        {
            return m_resource;
        }
    private:
        VkPipeline m_resource;
    };
    class VulkanPipelineCache : public RHIPipelineCache
    {
    public:
        void SetResource(VkPipelineCache res)
        {
            m_resource = res;
        }
        VkPipelineCache GetResource() const
        {
            return m_resource;
        }
    private:
        VkPipelineCache m_resource;
    };
    class VulkanPipelineLayout : public RHIPipelineLayout
    {
    public:
        void SetResource(VkPipelineLayout res)
        {
            m_resource = res;
        }
        VkPipelineLayout GetResource() const
        {
            return m_resource;
        }
    private:
        VkPipelineLayout m_resource;
    };
    class VulkanRenderPass : public RHIRenderPass
    {
    public:
        void SetResource(VkRenderPass res)
        {
            m_resource = res;
        }
        VkRenderPass GetResource() const
        {
            return m_resource;
        }
    private:
        VkRenderPass m_resource;
    };
    class VulkanSampler : public RHISampler
    {
    public:
        void SetResource(VkSampler res)
        {
            m_resource = res;
        }
        VkSampler GetResource() const
        {
            return m_resource;
        }
    private:
        VkSampler m_resource;
    };
    class VulkanSemaphore : public RHISemaphore
    {
    public:
        void SetResource(VkSemaphore res)
        {
            m_resource = res;
        }
        VkSemaphore &GetResource()
        {
            return m_resource;
        }
    private:
        VkSemaphore m_resource;
    };
    class VulkanShader : public RHIShader
    {
    public:
        void SetResource(VkShaderModule res)
        {
            m_resource = res;
        }
        VkShaderModule GetResource() const
        {
            return m_resource;
        }
    private:
        VkShaderModule m_resource;
    };
}