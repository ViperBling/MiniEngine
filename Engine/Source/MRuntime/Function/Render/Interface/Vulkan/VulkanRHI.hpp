#pragma once

#include <cstring>
#include <set>
#include <vector>

#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"

#include "Function/Render/Interface/RHI.hpp"
#include "Function/Render/Interface/RHIStruct.hpp"
#include "Function/Render/RenderType.hpp"
#include "Function/Render/Interface/Vulkan/VulkanRHIResource.hpp"

namespace MiniEngine
{
    class VulkanRHI final : public RHI
    {
    public:
        void Initialize(RHIInitInfo initInfo) final;
        void PrepareContext() final;

        void CreateSwapChain() override;
        void CreateSwapChainImageViews() override;
        RHIShader* CreateShaderModule(const std::vector<unsigned char>& shaderCode) override;
        bool CreateGraphicsPipeline(
            RHIPipelineCache*                    pipelineCache,
            uint32_t                             createInfoCnt,
            const RHIGraphicsPipelineCreateInfo* pCreateInfo,
            RHIPipeline*&                        pPipelines) override;
            
        bool CreatePipelineLayout(
            const RHIPipelineLayoutCreateInfo* pCreateInfo,
            RHIPipelineLayout*&                pPipelineLayout) override;

        bool CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) override;
        bool CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer*& pFrameBuffer) override;
        void RecreateSwapChain() override;
        virtual void CreateBuffer(
            RHIDeviceSize           size,
            RHIBufferUsageFlags     usageFlags,
            RHIMemoryPropertyFlags  properties,
            RHIBuffer*&             buffer,
            RHIDeviceMemory*&       bufferMemory
        ) override;

        // command and command write
        virtual void CmdBindVertexBuffersPFN(
            RHICommandBuffer*       cmdBuffer,
            uint32_t                firstBinding,
            uint32_t                bindingCount,
            RHIBuffer* const*       pBuffers,
            const RHIDeviceSize*    pOffsets
        ) override;
        void CmdBeginRenderPassPFN(
            RHICommandBuffer*             commandBuffer,
            const RHIRenderPassBeginInfo* pRenderPassBegin,
            RHISubpassContents            contents
        ) override;
        void CmdBindPipelinePFN(
            RHICommandBuffer*    commandBuffer,
            RHIPipelineBindPoint pipelineBindPoint,
            RHIPipeline*         pipeline
        ) override;
        void CmdDraw(
            RHICommandBuffer* commandBuffer,
            uint32_t          vertexCount,
            uint32_t          instanceCount,
            uint32_t          firstVertex,
            uint32_t          firstInstance
        ) override;
        void CmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) override;
        void CmdSetViewportPFN(
            RHICommandBuffer*  commandBuffer,
            uint32_t           firstViewport,
            uint32_t           viewportCount,
            const RHIViewport* pViewports
        ) override;
        void CmdSetScissorPFN(
            RHICommandBuffer* commandBuffer,
            uint32_t          firstScissor,
            uint32_t          scissorCount,
            const RHIRect2D*  pScissors
        ) override;
        void WaitForFences() override;

        // Query
        RHISwapChainDesc GetSwapChainInfo() override;
        RHICommandBuffer* GetCurrentCommandBuffer() const override;

        // command write
        bool PrepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapChain) override;
        void SubmitRendering(std::function<void()> passUpdateAfterRecreateSwapChain) override;

        virtual void DestroyFrameBuffer(RHIFrameBuffer* frameBuffer) override;

        virtual bool MapMemory(
            RHIDeviceMemory* memory,
            RHIDeviceSize offset,
            RHIDeviceSize size,
            RHIMemoryMapFlags flags,
            void** ppData
        ) override;
        virtual void UnmapMemory(RHIDeviceMemory* memory) override;

    public:
        static uint8_t const mkMaxFramesInFlight {3};               // 最大同时渲染的图片数量

        RHIQueue* mGraphicsQueue {nullptr};                         // 图形队列句柄

        RHIFormat mSwapChainImageFormat {RHI_FORMAT_UNDEFINED};     // 交换链图片格式
        RHIExtent2D mSwapChainExtent;                               // 交换链图片范围
        std::vector<RHIImageView*> mSwapChainImageViews;            // 图像的视图：描述如何访问图像以及访问图像的哪一部分
        RHIViewport mViewport;
        RHIRect2D mScissor;

        RHICommandBuffer* mCommandBuffers[mkMaxFramesInFlight];

        QueueFamilyIndices mQueueIndices;                           // 队列家族索引

        GLFWwindow*         mWindow {nullptr};
        VkInstance          mInstance {nullptr};                    // Vulkan实体
        VkSurfaceKHR        mSurface {nullptr};                     // 窗口界面
        VkPhysicalDevice    mPhysicalDevice {nullptr};              // 显卡
        VkDevice            mDevice {nullptr};                      // 逻辑设备
        VkQueue             mPresentQueue {nullptr};

        RHICommandPool* mRHICommandPool;                            // 命令池
        RHICommandBuffer* mCurrentCommandBuffer = new VulkanCommandBuffer();

        VkSwapchainKHR          mSwapChain {nullptr};               // 交换链句柄
        std::vector<VkImage>    mSwapChainImages;                   // 交换链图像句柄

        // Command Pool and Buffers
        uint8_t             mCurrentFrameIndex {0};
        VkCommandPool       mCommandPools[mkMaxFramesInFlight];
        VkCommandBuffer     mVkCommandBuffers[mkMaxFramesInFlight];
        VkSemaphore         mImageAvailableForRenderSemaphore[mkMaxFramesInFlight];
        VkSemaphore         mImageFinishedForPresentationSemaphore[mkMaxFramesInFlight];
        VkFence             mIsFrameInFlightFences[mkMaxFramesInFlight];

        // TODO set
        VkCommandBuffer     mVkCurrentCommandBuffer;

        uint32_t            mCurrentSwapChainImageIndex;

        // function pointers
        PFN_vkBeginCommandBuffer    pfnVkBeginCommandBuffer;
        PFN_vkEndCommandBuffer      pfnVkEndCommandBuffer;
        PFN_vkCmdBindVertexBuffers  pfnVkCmdBindVertexBuffers;
        PFN_vkCmdBeginRenderPass    pfnVkCmdBeginRenderPass;
        PFN_vkCmdEndRenderPass      pfnVkCmdEndRenderPass;
        PFN_vkCmdBindPipeline       pfnVkCmdBindPipeline;
        PFN_vkCmdSetViewport        pfnVkCmdSetViewport;
        PFN_vkCmdSetScissor         pfnVkCmdSetScissor;
        PFN_vkWaitForFences         pfnVkWaitForFences;
        PFN_vkResetFences           pfnVkResetFences;

    private:
        void createInstance();
        void InitializeDebugMessenger();
        void createWindowSurface();
        void initializePhysicalDevice();
        void createLogicDevice();

        void createCommandPool();
        void createCommandBuffers();
        void createSyncPrimitive();

        bool checkValidationLayersSupport();
        std::vector<const char*> getRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        VkResult createDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks*              pAllocator,
            VkDebugUtilsMessengerEXT*                 pDebugMessenger);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
        bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
        bool isDeviceSuitable(VkPhysicalDevice physicalDevice);

        VkSurfaceFormatKHR chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
        VkPresentModeKHR chooseSwapChainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapChainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities);

    private:

        bool bEnableValidationLayers {true};                        // 启用验证层
        bool bEnableDebugUtilsLabel {true};

        VkDebugUtilsMessengerEXT mDebugMessenger {nullptr};

        const std::vector<char const*> mValidationLayers {"VK_LAYER_KHRONOS_validation"};
        uint32_t mVulkanAPIVersion {VK_API_VERSION_1_0};

        // 交换链插件：等待着被显示到屏幕上的图像的队列
        std::vector<char const*> mDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };
}