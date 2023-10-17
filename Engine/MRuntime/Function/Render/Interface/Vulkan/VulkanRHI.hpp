#pragma once

#include "Function/Render/Interface/RHI.hpp"
#include "Function/Render/Interface/RHIStruct.hpp"
#include "Function/Render/RenderType.hpp"
#include "Function/Render/Interface/Vulkan/VulkanRHIResource.hpp"

#include <cstring>
#include <set>
#include <vector>
#include <map>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace MiniEngine
{
    class VulkanRHI final : public RHI
    {
    public:
        virtual void Initialize(RHIInitInfo initInfo) override;
        virtual void PrepareContext()                 override;

        // allocate and create
        virtual bool AllocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer* &pCommandBuffers) override;
        virtual bool AllocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet* &pDescriptorSets) override;
        virtual void CreateSwapChain()                                                      override;
        virtual void CreateSwapChainImageViews()                                            override;
        virtual RHIShader* CreateShaderModule(const std::vector<unsigned char>& shaderCode) override;
        virtual void CreateFramebufferImageAndView() override;
        virtual RHISampler* GetOrCreateDefaultSampler(RHIDefaultSamplerType type) override;
        virtual RHISampler* GetOrCreateMipmapSampler(uint32_t width, uint32_t height) override;
        virtual bool CreateGraphicsPipeline(RHIPipelineCache* pipelineCache, uint32_t createInfoCnt, const RHIGraphicsPipelineCreateInfo* pCreateInfo, RHIPipeline*& pPipelines)  override;
        virtual bool CreatePipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout) override;

        virtual bool CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) override;
        virtual bool CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer*& pFrameBuffer) override;
        virtual void RecreateSwapChain() override;
        virtual void CreateBuffer(RHIDeviceSize size, RHIBufferUsageFlags usageFlags, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& bufferMemory) override;
        virtual void CreateBufferAndInitialize(RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory, RHIDeviceSize size, void* data = nullptr, int datasize = 0) override;
        virtual bool CreateBufferVMA(VmaAllocator allocator, const RHIBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, RHIBuffer* &pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo) override;
        virtual bool CreateBufferWithAlignmentVMA(VmaAllocator allocator, const RHIBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, RHIDeviceSize minAlignment, RHIBuffer* &pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo) override;
        virtual void CopyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, RHIDeviceSize srcOffset, RHIDeviceSize dstOffset, RHIDeviceSize size) override;
        virtual void CreateImage(
            uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
            RHIImage* &image, RHIDeviceMemory* &memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels
        ) override;
        virtual void CreateImageView(RHIImage* image, RHIFormat format, RHIImageAspectFlags image_aspect_flags, RHIImageViewType view_type, uint32_t layout_count, uint32_t miplevels, RHIImageView* &image_view) override;
        virtual void CreateGlobalImage(RHIImage* &image, RHIImageView* &image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, void* texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels = 0) override;
        virtual void CreateCubeMap(RHIImage* &image, RHIImageView* &image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, std::array<void*, 6> texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels) override;
        virtual void CreateCommandPool() override;
        virtual bool CreateCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool*& pCommandPool) override;
        virtual bool CreateDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool* &pDescriptorPool) override;
        virtual bool CreateDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* pCreateInfo, RHIDescriptorSetLayout* &pSetLayout) override;
        virtual bool CreateFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence* &pFence) override;
        virtual bool CreateGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfos, RHIPipeline* &pPipelines) override;
        virtual bool CreateComputePipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIComputePipelineCreateInfo* pCreateInfos, RHIPipeline* &pPipelines) override;
        virtual bool CreateSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler* &pSampler) override;
        virtual bool CreateSemaphores(const RHISemaphoreCreateInfo* pCreateInfo, RHISemaphore*& pSemaphore) override;

        // command and command write
        virtual bool WaitForFencesPFN(uint32_t fenceCount, RHIFence* const* pFence, RHIBool32 waitAll, uint64_t timeout) override;
        virtual bool ResetFencesPFN(uint32_t fenceCount, RHIFence* const* pFences) override;
        virtual bool ResetCommandPoolPFN(RHICommandPool* commandPool, RHICommandPoolResetFlags flags) override;
        virtual bool BeginCommandBufferPFN(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;
        virtual bool EndCommandBufferPFN(RHICommandBuffer* commandBuffer) override;
        virtual void CmdBindVertexBuffersPFN(RHICommandBuffer* cmdBuffer, uint32_t firstBinding, uint32_t bindingCount, RHIBuffer* const* pBuffers, const RHIDeviceSize* pOffsets) override;
        virtual void CmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType) override;
        virtual void CmdBindDescriptorSetsPFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipelineLayout* layout, uint32_t firstSet, uint32_t descriptorSetCount, const RHIDescriptorSet* const* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) override;
        virtual void CmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer,  const RHIRenderPassBeginInfo* pRenderPassBegin,  RHISubpassContents contents) override;
        virtual void CmdNextSubpassPFN(RHICommandBuffer* commandBuffer, RHISubpassContents contents) override;
        virtual void CmdBindPipelinePFN(RHICommandBuffer* commandBuffer,  RHIPipelineBindPoint pipelineBindPoint,  RHIPipeline* pipeline) override;
        virtual void CmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        virtual void CmdDrawIndexed(RHICommandBuffer* commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
        virtual void CmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) override;
        virtual void CmdSetViewportPFN(RHICommandBuffer* commandBuffer,  uint32_t firstViewport,  uint32_t viewportCount,  const RHIViewport* pViewports) override;
        virtual void CmdSetScissorPFN(RHICommandBuffer* commandBuffer,  uint32_t firstScissor,  uint32_t scissorCount,  const RHIRect2D* pScissors) override;
        virtual void CmdClearAttachmentsPFN(RHICommandBuffer* commandBuffer, uint32_t attachmentCount, const RHIClearAttachment* pAttachments, uint32_t rectCount, const RHIClearRect* pRects) override;
        
        virtual bool BeginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;
        virtual void CmdCopyImageToBuffer(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageLayout srcImageLayout, RHIBuffer* dstBuffer, uint32_t regionCount, const RHIBufferImageCopy* pRegions) override;
        virtual void CmdCopyImageToImage(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageAspectFlagBits srcFlag, RHIImage* dstImage, RHIImageAspectFlagBits dstFlag, uint32_t width, uint32_t height) override;
        virtual void CmdCopyBuffer(RHICommandBuffer* commandBuffer, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32_t regionCount, RHIBufferCopy* pRegions) override;
        virtual void CmdDispatch(RHICommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        virtual void CmdDispatchIndirect(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset) override;
        virtual void CmdPipelineBarrier(RHICommandBuffer* commandBuffer, RHIPipelineStageFlags srcStageMask, RHIPipelineStageFlags dstStageMask, RHIDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const RHIMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const RHIBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const RHIImageMemoryBarrier* pImageMemoryBarriers) override;
        virtual bool EndCommandBuffer(RHICommandBuffer* commandBuffer) override;
        virtual void UpdateDescriptorSets(uint32_t descriptorWriteCount, const RHIWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const RHICopyDescriptorSet* pDescriptorCopies) override;
        virtual bool QueueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence) override;
        virtual bool QueueWaitIdle(RHIQueue* queue) override;
        virtual void ResetCommandPool() override;
        virtual void WaitForFences() override;
        bool WaitForFences(uint32_t fenceCount, const RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout);

        // query
        virtual void GetPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties) override;
        virtual RHICommandBuffer* GetCurrentCommandBuffer() const override;
        virtual RHICommandBuffer* const* GetCommandBufferList() const override;
        virtual RHICommandPool* GetCommandPool() const override;
        virtual RHIDescriptorPool* GetDescriptorPool() const override;
        virtual RHIFence* const* GetFenceList() const override;
        virtual QueueFamilyIndices GetQueueFamilyIndices() const override;
        virtual RHIQueue* GetGraphicsQueue() const override;
        virtual RHIQueue* GetComputeQueue() const override;
        virtual RHISwapChainDesc GetSwapChainInfo() override;
        virtual RHIDepthImageDesc GetDepthImageInfo() const override;
        virtual uint8_t GetMaxFramesInFlight() const override;
        virtual uint8_t GetCurrentFrameIndex() const override;
        virtual void SetCurrentFrameIndex(uint8_t index) override;

        // command write
        virtual bool PrepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapChain) override;
        virtual void SubmitRendering(std::function<void()> passUpdateAfterRecreateSwapChain)   override;
        virtual RHICommandBuffer* BeginSingleTimeCommand() override;
        virtual void EndSingleTimeCommand(RHICommandBuffer* cmdBuffer) override;
        virtual void PushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color) override;
        virtual void PopEvent(RHICommandBuffer* commond_buffer) override;

        // Destroy
        virtual void Clear() override;
        virtual void ClearSwapchain() override;
        virtual void DestroyDefaultSampler(RHIDefaultSamplerType type) override;
        virtual void DestroyMipmappedSampler() override;
        virtual void DestroyShaderModule(RHIShader* shader) override;
        virtual void DestroySemaphore(RHISemaphore* semaphore) override;
        virtual void DestroySampler(RHISampler* sampler) override;
        virtual void DestroyInstance(RHIInstance* instance) override;
        virtual void DestroyImageView(RHIImageView* imageView) override;
        virtual void DestroyImage(RHIImage* image) override;
        virtual void DestroyFrameBuffer(RHIFrameBuffer* framebuffer) override;
        virtual void DestroyFence(RHIFence* fence) override;
        virtual void DestroyDevice() override;
        virtual void DestroyCommandPool(RHICommandPool* commandPool) override;
        virtual void DestroyBuffer(RHIBuffer* &buffer) override;
        virtual void FreeCommandBuffers(RHICommandPool* commandPool, uint32_t commandBufferCount, RHICommandBuffer* pCommandBuffers) override;

        // Buffer Memory
        virtual bool MapMemory(RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size, RHIMemoryMapFlags flags, void** ppData) override;
        virtual void UnmapMemory(RHIDeviceMemory* memory) override;
        virtual void FreeMemory(RHIDeviceMemory* &memory)  override;
        virtual void InvalidateMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size) override;
        virtual void FlushMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size) override;

        //semaphores
        virtual RHISemaphore* &GetTextureCopySemaphore(uint32_t index) override;

    private:
        void createInstance();
        void InitializeDebugMessenger();
        void createWindowSurface();
        void initializePhysicalDevice();
        void createLogicalDevice();
        void createCommandBuffers();
        void createDescriptorPool();
        void createSyncPrimitives();
        void createAssetAllocator();

        bool checkValidationLayersSupport();
        std::vector<const char*> getRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        VkResult createDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks*              pAllocator,
            VkDebugUtilsMessengerEXT*                 pDebugMessenger);
        void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
        bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);
        bool isDeviceSuitable(VkPhysicalDevice physicalDevice);

        VkFormat findDepthFormat();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        VkSurfaceFormatKHR chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
        VkPresentModeKHR chooseSwapChainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapChainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities);

    public:
        static uint8_t const mkMaxFramesInFlight {3};               // 最大同时渲染的图片数量

        RHIQueue* mGraphicsQueue {nullptr};                         // 图形队列句柄
        RHIQueue* mComputeQueue{ nullptr };

        RHIFormat mSwapChainImageFormat {RHI_FORMAT_UNDEFINED};     // 交换链图片格式
        RHIExtent2D mSwapChainExtent;                               // 交换链图片范围
        std::vector<RHIImageView*> mSwapChainImageViews;            // 图像的视图：描述如何访问图像以及访问图像的哪一部分
        RHIViewport mViewport;
        RHIRect2D mScissor;

        RHIImage*        mDepthImage = new VulkanImage();
        RHIFormat mDepthImageFormat{ RHI_FORMAT_UNDEFINED };
        RHIImageView* mDepthImageView = new VulkanImageView();
        VkDeviceMemory   mDepthImageMemory {nullptr};

        RHIFence* mRHIIsFrameInFlightFences[mkMaxFramesInFlight];

        RHIDescriptorPool* mDescPool = new VulkanDescriptorPool();
        // global descriptor pool
        VkDescriptorPool mVkDescPool;

        RHICommandPool* mRHICommandPool;                            // 命令池
        RHICommandBuffer* mCurrentCommandBuffer = new VulkanCommandBuffer();
        RHICommandBuffer* mCommandBuffers[mkMaxFramesInFlight];

        QueueFamilyIndices mQueueIndices;                           // 队列家族索引

        GLFWwindow*         mWindow {nullptr};
        VkInstance          mInstance {nullptr};                    // Vulkan实体
        VkSurfaceKHR        mSurface {nullptr};                     // 窗口界面
        VkPhysicalDevice    mPhysicalDevice {nullptr};              // 显卡
        VkDevice            mDevice {nullptr};                      // 逻辑设备
        VkQueue             mPresentQueue {nullptr};

        VkSwapchainKHR          mSwapChain {nullptr};               // 交换链句柄
        std::vector<VkImage>    mSwapChainImages;                   // 交换链图像句柄

        std::vector<VkFramebuffer> mSwapChainFrameBuffers;

        // asset allocator use VMA library
        VmaAllocator mAssetsAllocator;

        // Command Pool and Buffers
        uint8_t             mCurrentFrameIndex {0};
        VkCommandPool       mCommandPools[mkMaxFramesInFlight];
        VkCommandBuffer     mVkCommandBuffers[mkMaxFramesInFlight];
        VkSemaphore         mImageAvailableForRenderSemaphores[mkMaxFramesInFlight];
        VkSemaphore         mImageFinishedForPresentationSemaphores[mkMaxFramesInFlight];
        RHISemaphore*       mImageAvailableForTextureCopySemaphores[mkMaxFramesInFlight];
        VkFence             mIsFrameInFlightFences[mkMaxFramesInFlight];

        // TODO set
        VkCommandBuffer     mVkCurrentCommandBuffer;

        uint32_t            mCurrentSwapChainImageIndex;

        // function pointers
        PFN_vkCmdBeginDebugUtilsLabelEXT pfnVkCmdBeginDebugUtilsLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT   pfnVkCmdEndDebugUtilsLabelEXT;
        PFN_vkWaitForFences              pfnVkWaitForFences;
        PFN_vkResetFences                pfnVkResetFences;
        PFN_vkResetCommandPool           pfnVkResetCommandPool;
        PFN_vkBeginCommandBuffer         pfnVkBeginCommandBuffer;
        PFN_vkEndCommandBuffer           pfnVkEndCommandBuffer;
        PFN_vkCmdBeginRenderPass         pfnVkCmdBeginRenderPass;
        PFN_vkCmdNextSubpass             pfnVkCmdNextSubpass;
        PFN_vkCmdEndRenderPass           pfnVkCmdEndRenderPass;
        PFN_vkCmdBindPipeline            pfnVkCmdBindPipeline;
        PFN_vkCmdSetViewport             pfnVkCmdSetViewport;
        PFN_vkCmdSetScissor              pfnVkCmdSetScissor;
        PFN_vkCmdBindVertexBuffers       pfnVkCmdBindVertexBuffers;
        PFN_vkCmdBindIndexBuffer         pfnVkCmdBindIndexBuffer;
        PFN_vkCmdBindDescriptorSets      pfnVkCmdBindDescriptorSets;
        PFN_vkCmdDrawIndexed             pfnVkCmdDrawIndexed;
        PFN_vkCmdClearAttachments        pfnVkCmdClearAttachments;

    private:
        bool mbEnableValidationLayers {true};                        // 启用验证层
        bool mbEnableDebugUtilsLabel {true};
        bool mbEnablePointLightShadow{ true };

        VkDebugUtilsMessengerEXT mDebugMessenger {nullptr};

        const std::vector<char const*> mValidationLayers {"VK_LAYER_KHRONOS_validation"};
        uint32_t mVulkanAPIVersion {VK_API_VERSION_1_1};

        // 交换链插件：等待着被显示到屏幕上的图像的队列
        std::vector<char const*> mDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // default sampler cache
        RHISampler* mLinearSampler = nullptr;
        RHISampler* mNearestSampler = nullptr;
        std::map<uint32_t, RHISampler*> mMipSamplerMap {};

        // used in descriptor pool creation
        uint32_t mMaxVertexBlendingMeshCount { 256 };
        uint32_t mMaxMaterialCount{ 256 };
    };
}