#if defined(__GNUC__)
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__linux__)
#include <stdlib.h>
#elif defined(__MACH__)
// https://developer.apple.com/library/archive/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html
#include <stdlib.h>
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
// https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS 1
#define NOVIRTUALKEYCODES 1
#define NOWINMESSAGES 1
#define NOWINSTYLES 1
#define NOSYSMETRICS 1
#define NOMENUS 1
#define NOICONS 1
#define NOKEYSTATES 1
#define NOSYSCOMMANDS 1
#define NORASTEROPS 1
#define NOSHOWWINDOW 1
#define NOATOM 1
#define NOCLIPBOARD 1
#define NOCOLOR 1
#define NOCTLMGR 1
#define NODRAWTEXT 1
#define NOGDI 1
#define NOKERNEL 1
#define NOUSER 1
#define NONLS 1
#define NOMB 1
#define NOMEMMGR 1
#define NOMETAFILE 1
#define NOMINMAX 1
#define NOMSG 1
#define NOOPENFILE 1
#define NOSCROLL 1
#define NOSERVICE 1
#define NOSOUND 1
#define NOTEXTMETRIC 1
#define NOWH 1
#define NOWINOFFSETS 1
#define NOCOMM 1
#define NOKANJI 1
#define NOHELP 1
#define NOPROFILER 1
#define NODEFERWINDOWPOS 1
#define NOMCX 1
#include <Windows.h>
#else
#error Unknown Compiler
#endif

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"

#include "VulkanRHI.hpp"

#include "MRuntime/Core/Base/Marco.hpp"
#include "VulkanRHIResource.hpp"
#include "VulkanUtil.hpp"
#include "MRuntime/Function/Render/WindowSystem.hpp"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <utility>

#define MINIENGINE_XSTR(s) MINIENGINE_STR(s)
#define MINIENGINE_STR(s) #s

namespace MiniEngine
{
    void VulkanRHI::Initialize(RHIInitInfo initInfo) {

        mWindow = initInfo.windowSystem->GetWindow();
        std::array<int, 2> windowSize = initInfo.windowSystem->GetWindowSize();

        // 视口初始化（详见视口变换与裁剪坐标）
        mViewport = {0.0f, 0.0f, static_cast<float>(windowSize[0]), static_cast<float>(windowSize[1]), 0.0f, 1.0f};
        // 裁剪坐标初始化
        mScissor = {{0, 0}, {static_cast<uint32_t>(windowSize[0]), static_cast<uint32_t>(windowSize[1])}};

        // 是否启用Debug
#ifndef NDEBUG
        mbEnableValidationLayers = true;
        mbEnableDebugUtilsLabel = true;
#else
        bEnableValidationLayers = false;
        bEnableDebugUtilsLabel = false;
#endif

#if defined(__GNUC__)
        // https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
#if defined(__linux__)
        char const* vk_layer_path = PICCOLO_XSTR(PICCOLO_VK_LAYER_PATH);
        setenv("VK_LAYER_PATH", vk_layer_path, 1);
#elif defined(__MACH__)
        // https://developer.apple.com/library/archive/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html
        char const* vk_layer_path    = PICCOLO_XSTR(PICCOLO_VK_LAYER_PATH);
        char const* vk_icd_filenames = PICCOLO_XSTR(PICCOLO_VK_ICD_FILENAMES);
        setenv("VK_LAYER_PATH", vk_layer_path, 1);
        setenv("VK_ICD_FILENAMES", vk_icd_filenames, 1);
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
        char const* vk_layer_path = MINIENGINE_XSTR(MINIENGINE_VK_LAYER_PATH);
        SetEnvironmentVariableA("VK_LAYER_PATH", vk_layer_path);
        SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
#else
#error Unknown Compiler
#endif

        // Vulkan初始化
        // 初始化Vulkan实例：设置应用名称、版本、拓展模块等
        createInstance();
        // 启动验证层从而在debug版本中发现可能存在的错误
        InitializeDebugMessenger();
        // 链接之前的glfw，使vulkan与当前运行平台窗口系统兼容
        createWindowSurface();
        // 选择物理设备，并进行一些兼容性检查
        initializePhysicalDevice();
        // 创建逻辑设备与准备队列，从而抽象你的物理设备为一些接口
        createLogicalDevice();

        CreateCommandPool();

        createCommandBuffers();
        createDescriptorPool();
        createSyncPrimitives();

        CreateSwapChain();
        CreateSwapChainImageViews();
        CreateFramebufferImageAndView();

        createAssetAllocator();
    }

    void VulkanRHI::PrepareContext() {
        mVkCurrentCommandBuffer = mVkCommandBuffers[mCurrentFrameIndex];
        static_cast<VulkanCommandBuffer*>(mCurrentCommandBuffer)->SetResource(mVkCurrentCommandBuffer);
    }

    void VulkanRHI::Clear()
    {
        if (mbEnableValidationLayers)
        {
            destroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
        }
    }

    void VulkanRHI::WaitForFences() 
    {
        VK_CHECK(pfnVkWaitForFences(mDevice, 1, &mIsFrameInFlightFences[mCurrentFrameIndex], VK_TRUE, UINT64_MAX)) 
    }

    bool VulkanRHI::WaitForFences(uint32_t fenceCount, const RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout)
    {
        //fence
        int fence_size = fenceCount;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = pFences[i];
            auto& vk_fence_element = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->GetResource();
        };

        VkResult result = vkWaitForFences(mDevice, fenceCount, vk_fence_list.data(), waitAll, timeout);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("waitForFences failed");
            return false;
        }
    }

    void VulkanRHI::GetPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties)
    {
        VkPhysicalDeviceProperties vk_physical_device_properties;
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &vk_physical_device_properties);

        pProperties->apiVersion = vk_physical_device_properties.apiVersion;
        pProperties->driverVersion = vk_physical_device_properties.driverVersion;
        pProperties->vendorID = vk_physical_device_properties.vendorID;
        pProperties->deviceID = vk_physical_device_properties.deviceID;
        pProperties->deviceType = (RHIPhysicalDeviceType)vk_physical_device_properties.deviceType;
        for (uint32_t i = 0; i < RHI_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++)
        {
            pProperties->deviceName[i] = vk_physical_device_properties.deviceName[i];
        }
        for (uint32_t i = 0; i < RHI_UUID_SIZE; i++)
        {
            pProperties->pipelineCacheUUID[i] = vk_physical_device_properties.pipelineCacheUUID[i];
        }
        pProperties->sparseProperties.residencyStandard2DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DBlockShape;
        pProperties->sparseProperties.residencyStandard2DMultisampleBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DMultisampleBlockShape;
        pProperties->sparseProperties.residencyStandard3DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard3DBlockShape;
        pProperties->sparseProperties.residencyAlignedMipSize = (VkBool32)vk_physical_device_properties.sparseProperties.residencyAlignedMipSize;
        pProperties->sparseProperties.residencyNonResidentStrict = (VkBool32)vk_physical_device_properties.sparseProperties.residencyNonResidentStrict;

        pProperties->limits.maxImageDimension1D = vk_physical_device_properties.limits.maxImageDimension1D;
        pProperties->limits.maxImageDimension2D = vk_physical_device_properties.limits.maxImageDimension2D;
        pProperties->limits.maxImageDimension3D = vk_physical_device_properties.limits.maxImageDimension3D;
        pProperties->limits.maxImageDimensionCube = vk_physical_device_properties.limits.maxImageDimensionCube;
        pProperties->limits.maxImageArrayLayers = vk_physical_device_properties.limits.maxImageArrayLayers;
        pProperties->limits.maxTexelBufferElements = vk_physical_device_properties.limits.maxTexelBufferElements;
        pProperties->limits.maxUniformBufferRange = vk_physical_device_properties.limits.maxUniformBufferRange;
        pProperties->limits.maxStorageBufferRange = vk_physical_device_properties.limits.maxStorageBufferRange;
        pProperties->limits.maxPushConstantsSize = vk_physical_device_properties.limits.maxPushConstantsSize;
        pProperties->limits.maxMemoryAllocationCount = vk_physical_device_properties.limits.maxMemoryAllocationCount;
        pProperties->limits.maxSamplerAllocationCount = vk_physical_device_properties.limits.maxSamplerAllocationCount;
        pProperties->limits.bufferImageGranularity = (VkDeviceSize)vk_physical_device_properties.limits.bufferImageGranularity;
        pProperties->limits.sparseAddressSpaceSize = (VkDeviceSize)vk_physical_device_properties.limits.sparseAddressSpaceSize;
        pProperties->limits.maxBoundDescriptorSets = vk_physical_device_properties.limits.maxBoundDescriptorSets;
        pProperties->limits.maxPerStageDescriptorSamplers = vk_physical_device_properties.limits.maxPerStageDescriptorSamplers;
        pProperties->limits.maxPerStageDescriptorUniformBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorUniformBuffers;
        pProperties->limits.maxPerStageDescriptorStorageBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
        pProperties->limits.maxPerStageDescriptorSampledImages = vk_physical_device_properties.limits.maxPerStageDescriptorSampledImages;
        pProperties->limits.maxPerStageDescriptorStorageImages = vk_physical_device_properties.limits.maxPerStageDescriptorStorageImages;
        pProperties->limits.maxPerStageDescriptorInputAttachments = vk_physical_device_properties.limits.maxPerStageDescriptorInputAttachments;
        pProperties->limits.maxPerStageResources = vk_physical_device_properties.limits.maxPerStageResources;
        pProperties->limits.maxDescriptorSetSamplers = vk_physical_device_properties.limits.maxDescriptorSetSamplers;
        pProperties->limits.maxDescriptorSetUniformBuffers = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffers;
        pProperties->limits.maxDescriptorSetUniformBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffersDynamic;
        pProperties->limits.maxDescriptorSetStorageBuffers = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffers;
        pProperties->limits.maxDescriptorSetStorageBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffersDynamic;
        pProperties->limits.maxDescriptorSetSampledImages = vk_physical_device_properties.limits.maxDescriptorSetSampledImages;
        pProperties->limits.maxDescriptorSetStorageImages = vk_physical_device_properties.limits.maxDescriptorSetStorageImages;
        pProperties->limits.maxDescriptorSetInputAttachments = vk_physical_device_properties.limits.maxDescriptorSetInputAttachments;
        pProperties->limits.maxVertexInputAttributes = vk_physical_device_properties.limits.maxVertexInputAttributes;
        pProperties->limits.maxVertexInputBindings = vk_physical_device_properties.limits.maxVertexInputBindings;
        pProperties->limits.maxVertexInputAttributeOffset = vk_physical_device_properties.limits.maxVertexInputAttributeOffset;
        pProperties->limits.maxVertexInputBindingStride = vk_physical_device_properties.limits.maxVertexInputBindingStride;
        pProperties->limits.maxVertexOutputComponents = vk_physical_device_properties.limits.maxVertexOutputComponents;
        pProperties->limits.maxTessellationGenerationLevel = vk_physical_device_properties.limits.maxTessellationGenerationLevel;
        pProperties->limits.maxTessellationPatchSize = vk_physical_device_properties.limits.maxTessellationPatchSize;
        pProperties->limits.maxTessellationControlPerVertexInputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexInputComponents;
        pProperties->limits.maxTessellationControlPerVertexOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexOutputComponents;
        pProperties->limits.maxTessellationControlPerPatchOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerPatchOutputComponents;
        pProperties->limits.maxTessellationControlTotalOutputComponents = vk_physical_device_properties.limits.maxTessellationControlTotalOutputComponents;
        pProperties->limits.maxTessellationEvaluationInputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationInputComponents;
        pProperties->limits.maxTessellationEvaluationOutputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationOutputComponents;
        pProperties->limits.maxGeometryShaderInvocations = vk_physical_device_properties.limits.maxGeometryShaderInvocations;
        pProperties->limits.maxGeometryInputComponents = vk_physical_device_properties.limits.maxGeometryInputComponents;
        pProperties->limits.maxGeometryOutputComponents = vk_physical_device_properties.limits.maxGeometryOutputComponents;
        pProperties->limits.maxGeometryOutputVertices = vk_physical_device_properties.limits.maxGeometryOutputVertices;
        pProperties->limits.maxGeometryTotalOutputComponents = vk_physical_device_properties.limits.maxGeometryTotalOutputComponents;
        pProperties->limits.maxFragmentInputComponents = vk_physical_device_properties.limits.maxFragmentInputComponents;
        pProperties->limits.maxFragmentOutputAttachments = vk_physical_device_properties.limits.maxFragmentOutputAttachments;
        pProperties->limits.maxFragmentDualSrcAttachments = vk_physical_device_properties.limits.maxFragmentDualSrcAttachments;
        pProperties->limits.maxFragmentCombinedOutputResources = vk_physical_device_properties.limits.maxFragmentCombinedOutputResources;
        pProperties->limits.maxComputeSharedMemorySize = vk_physical_device_properties.limits.maxComputeSharedMemorySize;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupCount[i] = vk_physical_device_properties.limits.maxComputeWorkGroupCount[i];
        }
        pProperties->limits.maxComputeWorkGroupInvocations = vk_physical_device_properties.limits.maxComputeWorkGroupInvocations;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupSize[i] = vk_physical_device_properties.limits.maxComputeWorkGroupSize[i];
        }
        pProperties->limits.subPixelPrecisionBits = vk_physical_device_properties.limits.subPixelPrecisionBits;
        pProperties->limits.subTexelPrecisionBits = vk_physical_device_properties.limits.subTexelPrecisionBits;
        pProperties->limits.mipmapPrecisionBits = vk_physical_device_properties.limits.mipmapPrecisionBits;
        pProperties->limits.maxDrawIndexedIndexValue = vk_physical_device_properties.limits.maxDrawIndexedIndexValue;
        pProperties->limits.maxDrawIndirectCount = vk_physical_device_properties.limits.maxDrawIndirectCount;
        pProperties->limits.maxSamplerLodBias = vk_physical_device_properties.limits.maxSamplerLodBias;
        pProperties->limits.maxSamplerAnisotropy = vk_physical_device_properties.limits.maxSamplerAnisotropy;
        pProperties->limits.maxViewports = vk_physical_device_properties.limits.maxViewports;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.maxViewportDimensions[i] = vk_physical_device_properties.limits.maxViewportDimensions[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.viewportBoundsRange[i] = vk_physical_device_properties.limits.viewportBoundsRange[i];
        }
        pProperties->limits.viewportSubPixelBits = vk_physical_device_properties.limits.viewportSubPixelBits;
        pProperties->limits.minMemoryMapAlignment = vk_physical_device_properties.limits.minMemoryMapAlignment;
        pProperties->limits.minTexelBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minTexelBufferOffsetAlignment;
        pProperties->limits.minUniformBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minUniformBufferOffsetAlignment;
        pProperties->limits.minStorageBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minStorageBufferOffsetAlignment;
        pProperties->limits.minTexelOffset = vk_physical_device_properties.limits.minTexelOffset;
        pProperties->limits.maxTexelOffset = vk_physical_device_properties.limits.maxTexelOffset;
        pProperties->limits.minTexelGatherOffset = vk_physical_device_properties.limits.minTexelGatherOffset;
        pProperties->limits.maxTexelGatherOffset = vk_physical_device_properties.limits.maxTexelGatherOffset;
        pProperties->limits.minInterpolationOffset = vk_physical_device_properties.limits.minInterpolationOffset;
        pProperties->limits.maxInterpolationOffset = vk_physical_device_properties.limits.maxInterpolationOffset;
        pProperties->limits.subPixelInterpolationOffsetBits = vk_physical_device_properties.limits.subPixelInterpolationOffsetBits;
        pProperties->limits.maxFramebufferWidth = vk_physical_device_properties.limits.maxFramebufferWidth;
        pProperties->limits.maxFramebufferHeight = vk_physical_device_properties.limits.maxFramebufferHeight;
        pProperties->limits.maxFramebufferLayers = vk_physical_device_properties.limits.maxFramebufferLayers;
        pProperties->limits.framebufferColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferColorSampleCounts;
        pProperties->limits.framebufferDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferDepthSampleCounts;
        pProperties->limits.framebufferStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferStencilSampleCounts;
        pProperties->limits.framebufferNoAttachmentsSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferNoAttachmentsSampleCounts;
        pProperties->limits.maxColorAttachments = vk_physical_device_properties.limits.maxColorAttachments;
        pProperties->limits.sampledImageColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageColorSampleCounts;
        pProperties->limits.sampledImageIntegerSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageIntegerSampleCounts;
        pProperties->limits.sampledImageDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageDepthSampleCounts;
        pProperties->limits.sampledImageStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageStencilSampleCounts;
        pProperties->limits.storageImageSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.storageImageSampleCounts;
        pProperties->limits.maxSampleMaskWords = vk_physical_device_properties.limits.maxSampleMaskWords;
        pProperties->limits.timestampComputeAndGraphics = (VkBool32)vk_physical_device_properties.limits.timestampComputeAndGraphics;
        pProperties->limits.timestampPeriod = vk_physical_device_properties.limits.timestampPeriod;
        pProperties->limits.maxClipDistances = vk_physical_device_properties.limits.maxClipDistances;
        pProperties->limits.maxCullDistances = vk_physical_device_properties.limits.maxCullDistances;
        pProperties->limits.maxCombinedClipAndCullDistances = vk_physical_device_properties.limits.maxCombinedClipAndCullDistances;
        pProperties->limits.discreteQueuePriorities = vk_physical_device_properties.limits.discreteQueuePriorities;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.pointSizeRange[i] = vk_physical_device_properties.limits.pointSizeRange[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.lineWidthRange[i] = vk_physical_device_properties.limits.lineWidthRange[i];
        }
        pProperties->limits.pointSizeGranularity = vk_physical_device_properties.limits.pointSizeGranularity;
        pProperties->limits.lineWidthGranularity = vk_physical_device_properties.limits.lineWidthGranularity;
        pProperties->limits.strictLines = (VkBool32)vk_physical_device_properties.limits.strictLines;
        pProperties->limits.standardSampleLocations = (VkBool32)vk_physical_device_properties.limits.standardSampleLocations;
        pProperties->limits.optimalBufferCopyOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
        pProperties->limits.optimalBufferCopyRowPitchAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
        pProperties->limits.nonCoherentAtomSize = (VkDeviceSize)vk_physical_device_properties.limits.nonCoherentAtomSize;
    }

    void VulkanRHI::ResetCommandPool()
    {
        VK_CHECK(pfnVkResetCommandPool(mDevice, mCommandPools[mCurrentFrameIndex], 0));
    }

    bool VulkanRHI::PrepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        VkResult acquire_image_result =
            vkAcquireNextImageKHR(mDevice,
                                  mSwapChain,
                                  UINT64_MAX,
                                  mImageAvailableForRenderSemaphores[mCurrentFrameIndex],
                                  VK_NULL_HANDLE,
                                  &mCurrentSwapChainImageIndex);

        if (VK_ERROR_OUT_OF_DATE_KHR == acquire_image_result)
        {
            RecreateSwapChain();
            passUpdateAfterRecreateSwapchain();
            return RHI_SUCCESS;
        }
        else if (VK_SUBOPTIMAL_KHR == acquire_image_result)
        {
            RecreateSwapChain();
            passUpdateAfterRecreateSwapchain();

            // NULL submit to wait semaphore
            VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            VkSubmitInfo         submit_info   = {};
            submit_info.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount     = 1;
            submit_info.pWaitSemaphores        = &mImageAvailableForRenderSemaphores[mCurrentFrameIndex];
            submit_info.pWaitDstStageMask      = wait_stages;
            submit_info.commandBufferCount     = 0;
            submit_info.pCommandBuffers        = NULL;
            submit_info.signalSemaphoreCount   = 0;
            submit_info.pSignalSemaphores      = NULL;

            VK_CHECK(pfnVkResetFences(mDevice, 1, &mIsFrameInFlightFences[mCurrentFrameIndex]))
            VK_CHECK(vkQueueSubmit(((VulkanQueue*)mGraphicsQueue)->GetResource(), 1, &submit_info, mIsFrameInFlightFences[mCurrentFrameIndex]))

            mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mkMaxFramesInFlight;
            return RHI_SUCCESS;
        }
        else
        {
            if (VK_SUCCESS != acquire_image_result)
            {
                LOG_ERROR("vkAcquireNextImageKHR failed!");
                return false;
            }
        }

        // begin command buffer
        VkCommandBufferBeginInfo command_buffer_begin_info {};
        command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags            = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        VK_CHECK(pfnVkBeginCommandBuffer(mVkCommandBuffers[mCurrentFrameIndex], &command_buffer_begin_info))

        return false;
    }

    void VulkanRHI::SubmitRendering(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        // end command buffer
        VkResult res_end_command_buffer = pfnVkEndCommandBuffer(mVkCommandBuffers[mCurrentFrameIndex]);
        if (VK_SUCCESS != res_end_command_buffer)
        {
            LOG_ERROR("pfnVkEndCommandBuffer failed!");
            return;
        }

        VkSemaphore semaphores[2] = { ((VulkanSemaphore*)mImageAvailableForTextureCopySemaphores[mCurrentFrameIndex])->GetResource(),
                                     mImageFinishedForPresentationSemaphores[mCurrentFrameIndex] };

        // submit command buffer
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo         submit_info   = {};
        submit_info.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount     = 1;
        submit_info.pWaitSemaphores        = &mImageAvailableForRenderSemaphores[mCurrentFrameIndex];
        submit_info.pWaitDstStageMask      = wait_stages;
        submit_info.commandBufferCount     = 1;
        submit_info.pCommandBuffers        = &mVkCommandBuffers[mCurrentFrameIndex];
        submit_info.signalSemaphoreCount = 2;
        submit_info.pSignalSemaphores = semaphores;
        VK_CHECK(pfnVkResetFences(mDevice, 1, &mIsFrameInFlightFences[mCurrentFrameIndex]))
        
        VK_CHECK(vkQueueSubmit(((VulkanQueue*)mGraphicsQueue)->GetResource(), 1, &submit_info, mIsFrameInFlightFences[mCurrentFrameIndex]))

        // present swapchain
        VkPresentInfoKHR present_info   = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &mImageFinishedForPresentationSemaphores[mCurrentFrameIndex];
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &mSwapChain;
        present_info.pImageIndices      = &mCurrentSwapChainImageIndex;

        VkResult present_result = vkQueuePresentKHR(mPresentQueue, &present_info);
        if (VK_ERROR_OUT_OF_DATE_KHR == present_result || VK_SUBOPTIMAL_KHR == present_result)
        {
            RecreateSwapChain();
            passUpdateAfterRecreateSwapchain();
        }
        else
        {
            if (VK_SUCCESS != present_result)
            {
                LOG_ERROR("vkQueuePresentKHR failed!");
                return;
            }
        }

        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mkMaxFramesInFlight;
    }

    RHICommandBuffer* VulkanRHI::BeginSingleTimeCommand()
    {
        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = ((VulkanCommandPool*)mRHICommandPool)->GetResource();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(mDevice, &allocInfo, &command_buffer);

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        pfnVkBeginCommandBuffer(command_buffer, &beginInfo);

        RHICommandBuffer* rhi_command_buffer = new VulkanCommandBuffer();
        ((VulkanCommandBuffer*)rhi_command_buffer)->SetResource(command_buffer);
        return rhi_command_buffer;
    }

    void VulkanRHI::EndSingleTimeCommand(RHICommandBuffer* command_buffer)
    {
        VkCommandBuffer vk_command_buffer = ((VulkanCommandBuffer*)command_buffer)->GetResource();
        pfnVkEndCommandBuffer(vk_command_buffer);

        VkSubmitInfo submitInfo {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &vk_command_buffer;

        vkQueueSubmit(((VulkanQueue*)mGraphicsQueue)->GetResource(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(((VulkanQueue*)mGraphicsQueue)->GetResource());

        vkFreeCommandBuffers(mDevice, ((VulkanCommandPool*)mRHICommandPool)->GetResource(), 1, &vk_command_buffer);
        delete(command_buffer);
    }

    // validation layers
    bool VulkanRHI::checkValidationLayersSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : mValidationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return RHI_SUCCESS;
    }

    std::vector<const char*> VulkanRHI::getRequiredExtensions()
    {
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (mbEnableValidationLayers || mbEnableDebugUtilsLabel)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

#if defined(__MACH__)
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        return extensions;
    }

    // debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                        VkDebugUtilsMessageTypeFlagsEXT,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void*)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    void VulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo       = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void VulkanRHI::createInstance()
    {
        // validation layer will be enabled in debug mode
        if (mbEnableValidationLayers && !checkValidationLayersSupport())
        {
            LOG_ERROR("validation layers requested, but not available!");
        }

        mVulkanAPIVersion = VK_API_VERSION_1_0;

        // app info
        VkApplicationInfo appInfo {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "MiniEngineRenderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "MiniEngine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = mVulkanAPIVersion;

        // create info
        VkInstanceCreateInfo instance_create_info {};
        instance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &appInfo; // the appInfo is stored here

        auto extensions                              = getRequiredExtensions();
        instance_create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
        if (mbEnableValidationLayers)
        {
            instance_create_info.enabledLayerCount   = static_cast<uint32_t>(mValidationLayers.size());
            instance_create_info.ppEnabledLayerNames = mValidationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.pNext             = nullptr;
        }

        // create m_vulkan_context._instance
        if (vkCreateInstance(&instance_create_info, nullptr, &mInstance) != VK_SUCCESS)
        {
            LOG_ERROR("vk create instance");
        }
    }

    void VulkanRHI::InitializeDebugMessenger()
    {
        if (mbEnableValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            populateDebugMessengerCreateInfo(createInfo);
            if (VK_SUCCESS != createDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger))
            {
                LOG_ERROR("failed to set up debug messenger!");
            }
        }

        if (mbEnableDebugUtilsLabel)
        {
            pfnVkCmdBeginDebugUtilsLabelEXT =
                (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(mInstance, "vkCmdBeginDebugUtilsLabelEXT");
            pfnVkCmdEndDebugUtilsLabelEXT =
                (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(mInstance, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    void VulkanRHI::createWindowSurface()
    {
        if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
        {
            LOG_ERROR("glfwCreateWindowSurface failed!");
        }
    }

    void VulkanRHI::initializePhysicalDevice()
    {
        uint32_t physical_device_count;
        vkEnumeratePhysicalDevices(mInstance, &physical_device_count, nullptr);
        if (physical_device_count == 0)
        {
            LOG_ERROR("enumerate physical devices failed!");
        }
        else
        {
            // find one device that matches our requirement
            // or find which is the best
            std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
            vkEnumeratePhysicalDevices(mInstance, &physical_device_count, physical_devices.data());

            std::vector<std::pair<int, VkPhysicalDevice>> ranked_physical_devices;
            for (const auto& device : physical_devices)
            {
                VkPhysicalDeviceProperties physical_device_properties;
                vkGetPhysicalDeviceProperties(device, &physical_device_properties);
                int score = 0;

                if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    score += 1000;
                }
                else if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                    score += 100;
                }

                ranked_physical_devices.push_back({score, device});
            }

            std::sort(ranked_physical_devices.begin(),
                      ranked_physical_devices.end(),
                      [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2) {
                          return p1 > p2;
                      });

            for (const auto& device : ranked_physical_devices)
            {
                if (isDeviceSuitable(device.second))
                {
                    mPhysicalDevice = device.second;
                    break;
                }
            }

            if (mPhysicalDevice == VK_NULL_HANDLE)
            {
                LOG_ERROR("failed to find suitable physical device");
            }
        }
    }

    // logical device (m_vulkan_context._device : graphic queue, present queue,
    // feature:samplerAnisotropy)
    void VulkanRHI::createLogicalDevice()
    {
        mQueueIndices = findQueueFamilies(mPhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos; // all queues that need to be created
        std::set<uint32_t>                   queue_families = {mQueueIndices.graphicsFamily.value(),
                                             mQueueIndices.presentFamily.value(),
                                             mQueueIndices.computeFamily.value()};

        float queue_priority = 1.0f;
        for (uint32_t queue_family : queue_families) // for every queue family
        {
            // queue create info
            VkDeviceQueueCreateInfo queue_create_info {};
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount       = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        // physical device features
        VkPhysicalDeviceFeatures physical_device_features = {};

        physical_device_features.samplerAnisotropy = VK_TRUE;

        // support inefficient readback storage buffer
        physical_device_features.fragmentStoresAndAtomics = VK_TRUE;

        // support independent blending
        physical_device_features.independentBlend = VK_TRUE;

        // support geometry shader
        if (mbEnablePointLightShadow)
        {
            physical_device_features.geometryShader = VK_TRUE;
        }

        // device create info
        VkDeviceCreateInfo device_create_info {};
        device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos       = queue_create_infos.data();
        device_create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pEnabledFeatures        = &physical_device_features;
        device_create_info.enabledExtensionCount   = static_cast<uint32_t>(mDeviceExtensions.size());
        device_create_info.ppEnabledExtensionNames = mDeviceExtensions.data();
        device_create_info.enabledLayerCount       = 0;

        if (vkCreateDevice(mPhysicalDevice, &device_create_info, nullptr, &mDevice) != VK_SUCCESS)
        {
            LOG_ERROR("vk create device");
        }

        // initialize queues of this device
        VkQueue vk_graphics_queue;
        vkGetDeviceQueue(mDevice, mQueueIndices.graphicsFamily.value(), 0, &vk_graphics_queue);
        mGraphicsQueue = new VulkanQueue();
        ((VulkanQueue*)mGraphicsQueue)->SetResource(vk_graphics_queue);

        vkGetDeviceQueue(mDevice, mQueueIndices.presentFamily.value(), 0, &mPresentQueue);

        VkQueue vk_compute_queue;
        vkGetDeviceQueue(mDevice, mQueueIndices.computeFamily.value(), 0, &vk_compute_queue);
        mComputeQueue = new VulkanQueue();
        ((VulkanQueue*)mComputeQueue)->SetResource(vk_compute_queue);

        // more efficient pointer
        pfnVkResetCommandPool      = (PFN_vkResetCommandPool)vkGetDeviceProcAddr(mDevice, "vkResetCommandPool");
        pfnVkBeginCommandBuffer    = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(mDevice, "vkBeginCommandBuffer");
        pfnVkEndCommandBuffer      = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(mDevice, "vkEndCommandBuffer");
        pfnVkCmdBeginRenderPass    = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(mDevice, "vkCmdBeginRenderPass");
        pfnVkCmdNextSubpass        = (PFN_vkCmdNextSubpass)vkGetDeviceProcAddr(mDevice, "vkCmdNextSubpass");
        pfnVkCmdEndRenderPass      = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(mDevice, "vkCmdEndRenderPass");
        pfnVkCmdBindPipeline       = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(mDevice, "vkCmdBindPipeline");
        pfnVkCmdSetViewport        = (PFN_vkCmdSetViewport)vkGetDeviceProcAddr(mDevice, "vkCmdSetViewport");
        pfnVkCmdSetScissor         = (PFN_vkCmdSetScissor)vkGetDeviceProcAddr(mDevice, "vkCmdSetScissor");
        pfnVkWaitForFences         = (PFN_vkWaitForFences)vkGetDeviceProcAddr(mDevice, "vkWaitForFences");
        pfnVkResetFences           = (PFN_vkResetFences)vkGetDeviceProcAddr(mDevice, "vkResetFences");
        pfnVkCmdDrawIndexed        = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(mDevice, "vkCmdDrawIndexed");
        pfnVkCmdBindVertexBuffers  = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(mDevice, "vkCmdBindVertexBuffers");
        pfnVkCmdBindIndexBuffer    = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(mDevice, "vkCmdBindIndexBuffer");
        pfnVkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(mDevice, "vkCmdBindDescriptorSets");
        pfnVkCmdClearAttachments   = (PFN_vkCmdClearAttachments)vkGetDeviceProcAddr(mDevice, "vkCmdClearAttachments");

        mDepthImageFormat = (RHIFormat)findDepthFormat();
    }

    void VulkanRHI::CreateCommandPool()
    {
        // default graphics command pool
        {
            mRHICommandPool = new VulkanCommandPool();
            VkCommandPool vk_command_pool;
            VkCommandPoolCreateInfo command_pool_create_info {};
            command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.pNext            = NULL;
            command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            command_pool_create_info.queueFamilyIndex = mQueueIndices.graphicsFamily.value();

            if (vkCreateCommandPool(mDevice, &command_pool_create_info, nullptr, &vk_command_pool) != VK_SUCCESS)
            {
                LOG_ERROR("vk create command pool");
            }

            ((VulkanCommandPool*)mRHICommandPool)->SetResource(vk_command_pool);
        }

        // other command pools
        {
            VkCommandPoolCreateInfo command_pool_create_info;
            command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_create_info.pNext            = NULL;
            command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            command_pool_create_info.queueFamilyIndex = mQueueIndices.graphicsFamily.value();

            for (uint32_t i = 0; i < mkMaxFramesInFlight; ++i)
            {
                if (vkCreateCommandPool(mDevice, &command_pool_create_info, NULL, &mCommandPools[i]) != VK_SUCCESS)
                {
                    LOG_ERROR("vk create command pool");
                }
            }
        }
    }

    bool VulkanRHI::CreateCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool* &pCommandPool)
    {
        VkCommandPoolCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkCommandPoolCreateFlags)pCreateInfo->flags;
        create_info.queueFamilyIndex = pCreateInfo->queueFamilyIndex;

        pCommandPool = new VulkanCommandPool();
        VkCommandPool vk_commandPool;
        VkResult result = vkCreateCommandPool(mDevice, &create_info, nullptr, &vk_commandPool);
        ((VulkanCommandPool*)pCommandPool)->SetResource(vk_commandPool);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateCommandPool is failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool* & pDescriptorPool)
    {
        int size = pCreateInfo->poolSizeCount;
        std::vector<VkDescriptorPoolSize> descriptor_pool_size(size);
        for (int i = 0; i < size; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pPoolSizes[i];
            auto& vk_desc = descriptor_pool_size[i];

            vk_desc.type = (VkDescriptorType)rhi_desc.type;
            vk_desc.descriptorCount = rhi_desc.descriptorCount;
        };

        VkDescriptorPoolCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkDescriptorPoolCreateFlags)pCreateInfo->flags;
        create_info.maxSets = pCreateInfo->maxSets;
        create_info.poolSizeCount = pCreateInfo->poolSizeCount;
        create_info.pPoolSizes = descriptor_pool_size.data();

        pDescriptorPool = new VulkanDescriptorPool();
        VkDescriptorPool vk_descriptorPool;
        VkResult result = vkCreateDescriptorPool(mDevice, &create_info, nullptr, &vk_descriptorPool);
        ((VulkanDescriptorPool*)pDescriptorPool)->SetResource(vk_descriptorPool);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateDescriptorPool is failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* pCreateInfo, RHIDescriptorSetLayout* &pSetLayout)
    {
        //descriptor_set_layout_binding
        int descriptor_set_layout_binding_size = pCreateInfo->bindingCount;
        std::vector<VkDescriptorSetLayoutBinding> vk_descriptor_set_layout_binding_list(descriptor_set_layout_binding_size);

        int sampler_count = 0;
        for (int i = 0; i < descriptor_set_layout_binding_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_binding_element = pCreateInfo->pBindings[i];
            if (rhi_descriptor_set_layout_binding_element.pImmutableSamplers != nullptr)
            {
                sampler_count += rhi_descriptor_set_layout_binding_element.descriptorCount;
            }
        }
        std::vector<VkSampler> sampler_list(sampler_count);
        int sampler_current = 0;

        for (int i = 0; i < descriptor_set_layout_binding_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_binding_element = pCreateInfo->pBindings[i];
            auto& vk_descriptor_set_layout_binding_element = vk_descriptor_set_layout_binding_list[i];

            //sampler
            vk_descriptor_set_layout_binding_element.pImmutableSamplers = nullptr;
            if (rhi_descriptor_set_layout_binding_element.pImmutableSamplers)
            {
                vk_descriptor_set_layout_binding_element.pImmutableSamplers = &sampler_list[sampler_current];
                for (int i = 0; i < rhi_descriptor_set_layout_binding_element.descriptorCount; ++i)
                {
                    const auto& rhi_sampler_element = rhi_descriptor_set_layout_binding_element.pImmutableSamplers[i];
                    auto& vk_sampler_element = sampler_list[sampler_current];

                    vk_sampler_element = ((VulkanSampler*)rhi_sampler_element)->GetResource();

                    sampler_current++;
                };
            }
            vk_descriptor_set_layout_binding_element.binding = rhi_descriptor_set_layout_binding_element.binding;
            vk_descriptor_set_layout_binding_element.descriptorType = (VkDescriptorType)rhi_descriptor_set_layout_binding_element.descriptorType;
            vk_descriptor_set_layout_binding_element.descriptorCount = rhi_descriptor_set_layout_binding_element.descriptorCount;
            vk_descriptor_set_layout_binding_element.stageFlags = rhi_descriptor_set_layout_binding_element.stageFlags;
        };
        
        if (sampler_count != sampler_current)
        {
            LOG_ERROR("sampler_count != sampller_current");
            return false;
        }

        VkDescriptorSetLayoutCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkDescriptorSetLayoutCreateFlags)pCreateInfo->flags;
        create_info.bindingCount = pCreateInfo->bindingCount;
        create_info.pBindings = vk_descriptor_set_layout_binding_list.data();

        pSetLayout = new VulkanDescriptorSetLayout();
        VkDescriptorSetLayout vk_descriptorSetLayout;
        VkResult result = vkCreateDescriptorSetLayout(mDevice, &create_info, nullptr, &vk_descriptorSetLayout);
        ((VulkanDescriptorSetLayout*)pSetLayout)->SetResource(vk_descriptorSetLayout);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateDescriptorSetLayout failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence* &pFence)
    {
        VkFenceCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkFenceCreateFlags)pCreateInfo->flags;

        pFence = new VulkanFence();
        VkFence vk_fence;
        VkResult result = vkCreateFence(mDevice, &create_info, nullptr, &vk_fence);
        ((VulkanFence*)pFence)->SetResource(vk_fence);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateFence failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer* &pFramebuffer)
    {
        //image_view
        int image_view_size = pCreateInfo->attachmentCount;
        std::vector<VkImageView> vk_image_view_list(image_view_size);
        for (int i = 0; i < image_view_size; ++i)
        {
            const auto& rhi_image_view_element = pCreateInfo->pAttachments[i];
            auto& vk_image_view_element = vk_image_view_list[i];

            vk_image_view_element = ((VulkanImageView*)rhi_image_view_element)->GetResource();
        };

        VkFramebufferCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkFramebufferCreateFlags)pCreateInfo->flags;
        create_info.renderPass = ((VulkanRenderPass*)pCreateInfo->renderPass)->GetResource();
        create_info.attachmentCount = pCreateInfo->attachmentCount;
        create_info.pAttachments = vk_image_view_list.data();
        create_info.width = pCreateInfo->width;
        create_info.height = pCreateInfo->height;
        create_info.layers = pCreateInfo->layers;

        pFramebuffer = new VulkanFrameBuffer();
        VkFramebuffer vk_framebuffer;
        VkResult result = vkCreateFramebuffer(mDevice, &create_info, nullptr, &vk_framebuffer);
        ((VulkanFrameBuffer*)pFramebuffer)->SetResource(vk_framebuffer);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateFramebuffer failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfo, RHIPipeline* &pPipelines)
    {
        //pipeline_shader_stage_create_info
        int pipeline_shader_stage_create_info_size = pCreateInfo->stageCount;
        std::vector<VkPipelineShaderStageCreateInfo> vk_pipeline_shader_stage_create_info_list(pipeline_shader_stage_create_info_size);

        int specialization_map_entry_size_total = 0;
        int specialization_info_total = 0;
        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                specialization_info_total++;
                specialization_map_entry_size_total+= rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
            }
        }
        std::vector<VkSpecializationInfo> vk_specialization_info_list(specialization_info_total);
        std::vector<VkSpecializationMapEntry> vk_specialization_map_entry_list(specialization_map_entry_size_total);
        int specialization_map_entry_current = 0;
        int specialization_info_current = 0;

        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            auto& vk_pipeline_shader_stage_create_info_element = vk_pipeline_shader_stage_create_info_list[i];

            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = &vk_specialization_info_list[specialization_info_current];

                VkSpecializationInfo vk_specialization_info{};
                vk_specialization_info.mapEntryCount = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                vk_specialization_info.pMapEntries = &vk_specialization_map_entry_list[specialization_map_entry_current];
                vk_specialization_info.dataSize = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->dataSize;
                vk_specialization_info.pData = (const void*)rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pData;

                //specialization_map_entry
                for (int i = 0; i < rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount; ++i)
                {
                    const auto& rhi_specialization_map_entry_element = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pMapEntries[i];
                    auto& vk_specialization_map_entry_element = vk_specialization_map_entry_list[specialization_map_entry_current];

                    vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element->constantID;
                    vk_specialization_map_entry_element.offset = rhi_specialization_map_entry_element->offset;
                    vk_specialization_map_entry_element.size = rhi_specialization_map_entry_element->size;

                    specialization_map_entry_current++;
                };

                specialization_info_current++;
            }
            else
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = nullptr;
            }
            vk_pipeline_shader_stage_create_info_element.sType = (VkStructureType)rhi_pipeline_shader_stage_create_info_element.sType;
            vk_pipeline_shader_stage_create_info_element.pNext = (const void*)rhi_pipeline_shader_stage_create_info_element.pNext;
            vk_pipeline_shader_stage_create_info_element.flags = (VkPipelineShaderStageCreateFlags)rhi_pipeline_shader_stage_create_info_element.flags;
            vk_pipeline_shader_stage_create_info_element.stage = (VkShaderStageFlagBits)rhi_pipeline_shader_stage_create_info_element.stage;
            vk_pipeline_shader_stage_create_info_element.module = ((VulkanShader*)rhi_pipeline_shader_stage_create_info_element.module)->GetResource();
            vk_pipeline_shader_stage_create_info_element.pName = rhi_pipeline_shader_stage_create_info_element.pName;
        };

        if (!((specialization_map_entry_size_total == specialization_map_entry_current)
            && (specialization_info_total == specialization_info_current)))
        {
            LOG_ERROR("(specialization_map_entry_size_total == specialization_map_entry_current)&& (specialization_info_total == specialization_info_current)");
            return false;
        }

        //vertex_input_binding_description
        int vertex_input_binding_description_size = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        std::vector<VkVertexInputBindingDescription> vk_vertex_input_binding_description_list(vertex_input_binding_description_size);
        for (int i = 0; i < vertex_input_binding_description_size; ++i)
        {
            const auto& rhi_vertex_input_binding_description_element = pCreateInfo->pVertexInputState->pVertexBindingDescriptions[i];
            auto& vk_vertex_input_binding_description_element = vk_vertex_input_binding_description_list[i];

            vk_vertex_input_binding_description_element.binding = rhi_vertex_input_binding_description_element.binding;
            vk_vertex_input_binding_description_element.stride = rhi_vertex_input_binding_description_element.stride;
            vk_vertex_input_binding_description_element.inputRate = (VkVertexInputRate)rhi_vertex_input_binding_description_element.inputRate;
        };

        //vertex_input_attribute_description
        int vertex_input_attribute_description_size = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        std::vector<VkVertexInputAttributeDescription> vk_vertex_input_attribute_description_list(vertex_input_attribute_description_size);
        for (int i = 0; i < vertex_input_attribute_description_size; ++i)
        {
            const auto& rhi_vertex_input_attribute_description_element = pCreateInfo->pVertexInputState->pVertexAttributeDescriptions[i];
            auto& vk_vertex_input_attribute_description_element = vk_vertex_input_attribute_description_list[i];

            vk_vertex_input_attribute_description_element.location = rhi_vertex_input_attribute_description_element.location;
            vk_vertex_input_attribute_description_element.binding = rhi_vertex_input_attribute_description_element.binding;
            vk_vertex_input_attribute_description_element.format = (VkFormat)rhi_vertex_input_attribute_description_element.format;
            vk_vertex_input_attribute_description_element.offset = rhi_vertex_input_attribute_description_element.offset;
        };

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        vk_pipeline_vertex_input_state_create_info.sType = (VkStructureType)pCreateInfo->pVertexInputState->sType;
        vk_pipeline_vertex_input_state_create_info.pNext = (const void*)pCreateInfo->pVertexInputState->pNext;
        vk_pipeline_vertex_input_state_create_info.flags = (VkPipelineVertexInputStateCreateFlags)pCreateInfo->pVertexInputState->flags;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vk_vertex_input_binding_description_list.data();
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_vertex_input_attribute_description_list.data();

        VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{};
        vk_pipeline_input_assembly_state_create_info.sType = (VkStructureType)pCreateInfo->pInputAssemblyState->sType;
        vk_pipeline_input_assembly_state_create_info.pNext = (const void*)pCreateInfo->pInputAssemblyState->pNext;
        vk_pipeline_input_assembly_state_create_info.flags = (VkPipelineInputAssemblyStateCreateFlags)pCreateInfo->pInputAssemblyState->flags;
        vk_pipeline_input_assembly_state_create_info.topology = (VkPrimitiveTopology)pCreateInfo->pInputAssemblyState->topology;
        vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = (VkBool32)pCreateInfo->pInputAssemblyState->primitiveRestartEnable;

        const VkPipelineTessellationStateCreateInfo* vk_pipeline_tessellation_state_create_info_ptr = nullptr;
        VkPipelineTessellationStateCreateInfo vk_pipeline_tessellation_state_create_info{};
        if (pCreateInfo->pTessellationState != nullptr)
        {
            vk_pipeline_tessellation_state_create_info.sType = (VkStructureType)pCreateInfo->pTessellationState->sType;
            vk_pipeline_tessellation_state_create_info.pNext = (const void*)pCreateInfo->pTessellationState->pNext;
            vk_pipeline_tessellation_state_create_info.flags = (VkPipelineTessellationStateCreateFlags)pCreateInfo->pTessellationState->flags;
            vk_pipeline_tessellation_state_create_info.patchControlPoints = pCreateInfo->pTessellationState->patchControlPoints;

            vk_pipeline_tessellation_state_create_info_ptr = &vk_pipeline_tessellation_state_create_info;
        }

        //viewport
        int viewport_size = pCreateInfo->pViewportState->viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = pCreateInfo->pViewportState->pViewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];

            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        };

        //rect_2d
        int rect_2d_size = pCreateInfo->pViewportState->scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = pCreateInfo->pViewportState->pScissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset2d{};
            offset2d.x = rhi_rect_2d_element.offset.x;
            offset2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extend2d{};
            extend2d.width = rhi_rect_2d_element.extent.width;
            extend2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = offset2d;
            vk_rect_2d_element.extent = extend2d;
        };

        VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{};
        vk_pipeline_viewport_state_create_info.sType = (VkStructureType)pCreateInfo->pViewportState->sType;
        vk_pipeline_viewport_state_create_info.pNext = (const void*)pCreateInfo->pViewportState->pNext;
        vk_pipeline_viewport_state_create_info.flags = (VkPipelineViewportStateCreateFlags)pCreateInfo->pViewportState->flags;
        vk_pipeline_viewport_state_create_info.viewportCount = pCreateInfo->pViewportState->viewportCount;
        vk_pipeline_viewport_state_create_info.pViewports = vk_viewport_list.data();
        vk_pipeline_viewport_state_create_info.scissorCount = pCreateInfo->pViewportState->scissorCount;
        vk_pipeline_viewport_state_create_info.pScissors = vk_rect_2d_list.data();

        VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{};
        vk_pipeline_rasterization_state_create_info.sType = (VkStructureType)pCreateInfo->pRasterizationState->sType;
        vk_pipeline_rasterization_state_create_info.pNext = (const void*)pCreateInfo->pRasterizationState->pNext;
        vk_pipeline_rasterization_state_create_info.flags = (VkPipelineRasterizationStateCreateFlags)pCreateInfo->pRasterizationState->flags;
        vk_pipeline_rasterization_state_create_info.depthClampEnable = (VkBool32)pCreateInfo->pRasterizationState->depthClampEnable;
        vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable = (VkBool32)pCreateInfo->pRasterizationState->rasterizerDiscardEnable;
        vk_pipeline_rasterization_state_create_info.polygonMode = (VkPolygonMode)pCreateInfo->pRasterizationState->polygonMode;
        vk_pipeline_rasterization_state_create_info.cullMode = (VkCullModeFlags)pCreateInfo->pRasterizationState->cullMode;
        vk_pipeline_rasterization_state_create_info.frontFace = (VkFrontFace)pCreateInfo->pRasterizationState->frontFace;
        vk_pipeline_rasterization_state_create_info.depthBiasEnable = (VkBool32)pCreateInfo->pRasterizationState->depthBiasEnable;
        vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor = pCreateInfo->pRasterizationState->depthBiasConstantFactor;
        vk_pipeline_rasterization_state_create_info.depthBiasClamp = pCreateInfo->pRasterizationState->depthBiasClamp;
        vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor = pCreateInfo->pRasterizationState->depthBiasSlopeFactor;
        vk_pipeline_rasterization_state_create_info.lineWidth = pCreateInfo->pRasterizationState->lineWidth;

        VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};
        vk_pipeline_multisample_state_create_info.sType = (VkStructureType)pCreateInfo->pMultisampleState->sType;
        vk_pipeline_multisample_state_create_info.pNext = (const void*)pCreateInfo->pMultisampleState->pNext;
        vk_pipeline_multisample_state_create_info.flags = (VkPipelineMultisampleStateCreateFlags)pCreateInfo->pMultisampleState->flags;
        vk_pipeline_multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)pCreateInfo->pMultisampleState->rasterizationSamples;
        vk_pipeline_multisample_state_create_info.sampleShadingEnable = (VkBool32)pCreateInfo->pMultisampleState->sampleShadingEnable;
        vk_pipeline_multisample_state_create_info.minSampleShading = pCreateInfo->pMultisampleState->minSampleShading;
        vk_pipeline_multisample_state_create_info.pSampleMask = (const RHISampleMask*)pCreateInfo->pMultisampleState->pSampleMask;
        vk_pipeline_multisample_state_create_info.alphaToCoverageEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToCoverageEnable;
        vk_pipeline_multisample_state_create_info.alphaToOneEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToOneEnable;

        VkStencilOpState stencil_op_state_front{};
        stencil_op_state_front.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.failOp;
        stencil_op_state_front.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.passOp;
        stencil_op_state_front.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.depthFailOp;
        stencil_op_state_front.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->front.compareOp;
        stencil_op_state_front.compareMask = pCreateInfo->pDepthStencilState->front.compareMask;
        stencil_op_state_front.writeMask = pCreateInfo->pDepthStencilState->front.writeMask;
        stencil_op_state_front.reference = pCreateInfo->pDepthStencilState->front.reference;

        VkStencilOpState stencil_op_state_back{};
        stencil_op_state_back.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.failOp;
        stencil_op_state_back.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.passOp;
        stencil_op_state_back.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.depthFailOp;
        stencil_op_state_back.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->back.compareOp;
        stencil_op_state_back.compareMask = pCreateInfo->pDepthStencilState->back.compareMask;
        stencil_op_state_back.writeMask = pCreateInfo->pDepthStencilState->back.writeMask;
        stencil_op_state_back.reference = pCreateInfo->pDepthStencilState->back.reference;


        VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{};
        vk_pipeline_depth_stencil_state_create_info.sType = (VkStructureType)pCreateInfo->pDepthStencilState->sType;
        vk_pipeline_depth_stencil_state_create_info.pNext = (const void*)pCreateInfo->pDepthStencilState->pNext;
        vk_pipeline_depth_stencil_state_create_info.flags = (VkPipelineDepthStencilStateCreateFlags)pCreateInfo->pDepthStencilState->flags;
        vk_pipeline_depth_stencil_state_create_info.depthTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthTestEnable;
        vk_pipeline_depth_stencil_state_create_info.depthWriteEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthWriteEnable;
        vk_pipeline_depth_stencil_state_create_info.depthCompareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->depthCompareOp;
        vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthBoundsTestEnable;
        vk_pipeline_depth_stencil_state_create_info.stencilTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->stencilTestEnable;
        vk_pipeline_depth_stencil_state_create_info.front = stencil_op_state_front;
        vk_pipeline_depth_stencil_state_create_info.back = stencil_op_state_back;
        vk_pipeline_depth_stencil_state_create_info.minDepthBounds = pCreateInfo->pDepthStencilState->minDepthBounds;
        vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = pCreateInfo->pDepthStencilState->maxDepthBounds;

        //pipeline_color_blend_attachment_state
        int pipeline_color_blend_attachment_state_size = pCreateInfo->pColorBlendState->attachmentCount;
        std::vector<VkPipelineColorBlendAttachmentState> vk_pipeline_color_blend_attachment_state_list(pipeline_color_blend_attachment_state_size);
        for (int i = 0; i < pipeline_color_blend_attachment_state_size; ++i)
        {
            const auto& rhi_pipeline_color_blend_attachment_state_element = pCreateInfo->pColorBlendState->pAttachments[i];
            auto& vk_pipeline_color_blend_attachment_state_element = vk_pipeline_color_blend_attachment_state_list[i];

            vk_pipeline_color_blend_attachment_state_element.blendEnable = (VkBool32)rhi_pipeline_color_blend_attachment_state_element.blendEnable;
            vk_pipeline_color_blend_attachment_state_element.srcColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.colorBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.colorBlendOp;
            vk_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.alphaBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.alphaBlendOp;
            vk_pipeline_color_blend_attachment_state_element.colorWriteMask = (VkColorComponentFlags)rhi_pipeline_color_blend_attachment_state_element.colorWriteMask;
        };

        VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{};
        vk_pipeline_color_blend_state_create_info.sType = (VkStructureType)pCreateInfo->pColorBlendState->sType;
        vk_pipeline_color_blend_state_create_info.pNext = pCreateInfo->pColorBlendState->pNext;
        vk_pipeline_color_blend_state_create_info.flags = pCreateInfo->pColorBlendState->flags;
        vk_pipeline_color_blend_state_create_info.logicOpEnable = pCreateInfo->pColorBlendState->logicOpEnable;
        vk_pipeline_color_blend_state_create_info.logicOp = (VkLogicOp)pCreateInfo->pColorBlendState->logicOp;
        vk_pipeline_color_blend_state_create_info.attachmentCount = pCreateInfo->pColorBlendState->attachmentCount;
        vk_pipeline_color_blend_state_create_info.pAttachments = vk_pipeline_color_blend_attachment_state_list.data();
        for (int i = 0; i < 4; ++i)
        {
            vk_pipeline_color_blend_state_create_info.blendConstants[i] = pCreateInfo->pColorBlendState->blendConstants[i];
        };

        //dynamic_state
        int dynamic_state_size = pCreateInfo->pDynamicState->dynamicStateCount;
        std::vector<VkDynamicState> vk_dynamic_state_list(dynamic_state_size);
        for (int i = 0; i < dynamic_state_size; ++i)
        {
            const auto& rhi_dynamic_state_element = pCreateInfo->pDynamicState->pDynamicStates[i];
            auto& vk_dynamic_state_element = vk_dynamic_state_list[i];

            vk_dynamic_state_element = (VkDynamicState)rhi_dynamic_state_element;
        };

        VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info{};
        vk_pipeline_dynamic_state_create_info.sType = (VkStructureType)pCreateInfo->pDynamicState->sType;
        vk_pipeline_dynamic_state_create_info.pNext = pCreateInfo->pDynamicState->pNext;
        vk_pipeline_dynamic_state_create_info.flags = (VkPipelineDynamicStateCreateFlags)pCreateInfo->pDynamicState->flags;
        vk_pipeline_dynamic_state_create_info.dynamicStateCount = pCreateInfo->pDynamicState->dynamicStateCount;
        vk_pipeline_dynamic_state_create_info.pDynamicStates = vk_dynamic_state_list.data();

        VkGraphicsPipelineCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkPipelineCreateFlags)pCreateInfo->flags;
        create_info.stageCount = pCreateInfo->stageCount;
        create_info.pStages = vk_pipeline_shader_stage_create_info_list.data();
        create_info.pVertexInputState = &vk_pipeline_vertex_input_state_create_info;
        create_info.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
        create_info.pTessellationState = vk_pipeline_tessellation_state_create_info_ptr;
        create_info.pViewportState = &vk_pipeline_viewport_state_create_info;
        create_info.pRasterizationState = &vk_pipeline_rasterization_state_create_info;
        create_info.pMultisampleState = &vk_pipeline_multisample_state_create_info;
        create_info.pDepthStencilState = &vk_pipeline_depth_stencil_state_create_info;
        create_info.pColorBlendState = &vk_pipeline_color_blend_state_create_info;
        create_info.pDynamicState = &vk_pipeline_dynamic_state_create_info;
        create_info.layout = ((VulkanPipelineLayout*)pCreateInfo->layout)->GetResource();
        create_info.renderPass = ((VulkanRenderPass*)pCreateInfo->renderPass)->GetResource();
        create_info.subpass = pCreateInfo->subpass;
        if (pCreateInfo->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)pCreateInfo->basePipelineHandle)->GetResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = pCreateInfo->basePipelineIndex;

        pPipelines = new VulkanPipeline();
        VkPipeline vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipelineCache != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipelineCache)->GetResource();
        }
        VkResult result = vkCreateGraphicsPipelines(mDevice, vk_pipeline_cache, createInfoCount, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)pPipelines)->SetResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateGraphicsPipelines failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateComputePipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIComputePipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines)
    {
        VkPipelineShaderStageCreateInfo shader_stage_create_info{};
        if (pCreateInfos->pStages->pSpecializationInfo != nullptr)
        {
            //will be complete soon if needed.
            shader_stage_create_info.pSpecializationInfo = nullptr;
        }
        else
        {
            shader_stage_create_info.pSpecializationInfo = nullptr;
        }
        shader_stage_create_info.sType = (VkStructureType)pCreateInfos->pStages->sType;
        shader_stage_create_info.pNext = (const void*)pCreateInfos->pStages->pNext;
        shader_stage_create_info.flags = (VkPipelineShaderStageCreateFlags)pCreateInfos->pStages->flags;
        shader_stage_create_info.stage = (VkShaderStageFlagBits)pCreateInfos->pStages->stage;
        shader_stage_create_info.module = ((VulkanShader*)pCreateInfos->pStages->module)->GetResource();
        shader_stage_create_info.pName = pCreateInfos->pStages->pName;

        VkComputePipelineCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfos->sType;
        create_info.pNext = (const void*)pCreateInfos->pNext;
        create_info.flags = (VkPipelineCreateFlags)pCreateInfos->flags;
        create_info.stage = shader_stage_create_info;
        create_info.layout = ((VulkanPipelineLayout*)pCreateInfos->layout)->GetResource();;
        if (pCreateInfos->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)pCreateInfos->basePipelineHandle)->GetResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = pCreateInfos->basePipelineIndex;

        pPipelines = new VulkanPipeline();
        VkPipeline vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipelineCache != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipelineCache)->GetResource();
        }
        VkResult result = vkCreateComputePipelines(mDevice, vk_pipeline_cache, createInfoCount, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)pPipelines)->SetResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateComputePipelines failed!");
            return false;
        }
    }

    bool VulkanRHI::CreatePipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout* &pPipelineLayout)
    {
        //descriptor_set_layout
        int descriptor_set_layout_size = pCreateInfo->setLayoutCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);
        for (int i = 0; i < descriptor_set_layout_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_element = pCreateInfo->pSetLayouts[i];
            auto& vk_descriptor_set_layout_element = vk_descriptor_set_layout_list[i];

            vk_descriptor_set_layout_element = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->GetResource();
        };

        VkPipelineLayoutCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkPipelineLayoutCreateFlags)pCreateInfo->flags;
        create_info.setLayoutCount = pCreateInfo->setLayoutCount;
        create_info.pSetLayouts = vk_descriptor_set_layout_list.data();

        pPipelineLayout = new VulkanPipelineLayout();
        VkPipelineLayout vk_pipeline_layout;
        VkResult result = vkCreatePipelineLayout(mDevice, &create_info, nullptr, &vk_pipeline_layout);
        ((VulkanPipelineLayout*)pPipelineLayout)->SetResource(vk_pipeline_layout);
        
        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreatePipelineLayout failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass* &pRenderPass)
    {
        // attachment convert
        std::vector<VkAttachmentDescription> vk_attachments(pCreateInfo->attachmentCount);
        for (int i = 0; i < pCreateInfo->attachmentCount; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pAttachments[i];
            auto& vk_desc = vk_attachments[i];

            vk_desc.flags = (VkAttachmentDescriptionFlags)(rhi_desc).flags;
            vk_desc.format = (VkFormat)(rhi_desc).format;
            vk_desc.samples = (VkSampleCountFlagBits)(rhi_desc).samples;
            vk_desc.loadOp = (VkAttachmentLoadOp)(rhi_desc).loadOp;
            vk_desc.storeOp = (VkAttachmentStoreOp)(rhi_desc).storeOp;
            vk_desc.stencilLoadOp = (VkAttachmentLoadOp)(rhi_desc).stencilLoadOp;
            vk_desc.stencilStoreOp = (VkAttachmentStoreOp)(rhi_desc).stencilStoreOp;
            vk_desc.initialLayout = (VkImageLayout)(rhi_desc).initialLayout;
            vk_desc.finalLayout = (VkImageLayout)(rhi_desc).finalLayout;
        };

        // subpass convert
        int totalAttachmentRefenrence = 0;
        for (int i = 0; i < pCreateInfo->subpassCount; i++)
        {
            const auto& rhi_desc = pCreateInfo->pSubpasses[i];
            totalAttachmentRefenrence += rhi_desc.inputAttachmentCount; // pInputAttachments
            totalAttachmentRefenrence += rhi_desc.colorAttachmentCount; // pColorAttachments
            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                totalAttachmentRefenrence += rhi_desc.colorAttachmentCount; // pDepthStencilAttachment
            }
            if (rhi_desc.pResolveAttachments != nullptr)
            {
                totalAttachmentRefenrence += rhi_desc.colorAttachmentCount; // pResolveAttachments
            }
        }
        std::vector<VkSubpassDescription> vk_subpass_description(pCreateInfo->subpassCount);
        std::vector<VkAttachmentReference> vk_attachment_reference(totalAttachmentRefenrence);
        int currentAttachmentRefence = 0;
        for (int i = 0; i < pCreateInfo->subpassCount; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pSubpasses[i];
            auto& vk_desc = vk_subpass_description[i];

            vk_desc.flags = (VkSubpassDescriptionFlags)(rhi_desc).flags;
            vk_desc.pipelineBindPoint = (VkPipelineBindPoint)(rhi_desc).pipelineBindPoint;
            vk_desc.preserveAttachmentCount = (rhi_desc).preserveAttachmentCount;
            vk_desc.pPreserveAttachments = (const uint32_t*)(rhi_desc).pPreserveAttachments;

            vk_desc.inputAttachmentCount = (rhi_desc).inputAttachmentCount;
            vk_desc.pInputAttachments = &vk_attachment_reference[currentAttachmentRefence];
            for (int i = 0; i < (rhi_desc).inputAttachmentCount; i++)
            {
                const auto& rhi_attachment_refence_input = (rhi_desc).pInputAttachments[i];
                auto& vk_attachment_refence_input = vk_attachment_reference[currentAttachmentRefence];

                vk_attachment_refence_input.attachment = rhi_attachment_refence_input.attachment;
                vk_attachment_refence_input.layout = (VkImageLayout)(rhi_attachment_refence_input.layout);

                currentAttachmentRefence += 1;
            };

            vk_desc.colorAttachmentCount = (rhi_desc).colorAttachmentCount;
            vk_desc.pColorAttachments = &vk_attachment_reference[currentAttachmentRefence];
            for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
            {
                const auto& rhi_attachment_refence_color = (rhi_desc).pColorAttachments[i];
                auto& vk_attachment_refence_color = vk_attachment_reference[currentAttachmentRefence];

                vk_attachment_refence_color.attachment = rhi_attachment_refence_color.attachment;
                vk_attachment_refence_color.layout = (VkImageLayout)(rhi_attachment_refence_color.layout);

                currentAttachmentRefence += 1;
            };

            if (rhi_desc.pResolveAttachments != nullptr)
            {
                vk_desc.pResolveAttachments = &vk_attachment_reference[currentAttachmentRefence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_resolve = (rhi_desc).pResolveAttachments[i];
                    auto& vk_attachment_refence_resolve = vk_attachment_reference[currentAttachmentRefence];

                    vk_attachment_refence_resolve.attachment = rhi_attachment_refence_resolve.attachment;
                    vk_attachment_refence_resolve.layout = (VkImageLayout)(rhi_attachment_refence_resolve.layout);

                    currentAttachmentRefence += 1;
                };
            }

            if (rhi_desc.pDepthStencilAttachment != nullptr)
            {
                vk_desc.pDepthStencilAttachment = &vk_attachment_reference[currentAttachmentRefence];
                for (int i = 0; i < (rhi_desc).colorAttachmentCount; ++i)
                {
                    const auto& rhi_attachment_refence_depth = (rhi_desc).pDepthStencilAttachment[i];
                    auto& vk_attachment_refence_depth = vk_attachment_reference[currentAttachmentRefence];

                    vk_attachment_refence_depth.attachment = rhi_attachment_refence_depth.attachment;
                    vk_attachment_refence_depth.layout = (VkImageLayout)(rhi_attachment_refence_depth.layout);

                    currentAttachmentRefence += 1;
                };
            };
        };
        if (currentAttachmentRefence != totalAttachmentRefenrence)
        {
            LOG_ERROR("currentAttachmentRefence != totalAttachmentRefenrence");
            return false;
        }

        std::vector<VkSubpassDependency> vk_subpass_depandecy(pCreateInfo->dependencyCount);
        for (int i = 0; i < pCreateInfo->dependencyCount; ++i)
        {
            const auto& rhi_desc = pCreateInfo->pDependencies[i];
            auto& vk_desc = vk_subpass_depandecy[i];

            vk_desc.srcSubpass = rhi_desc.srcSubpass;
            vk_desc.dstSubpass = rhi_desc.dstSubpass;
            vk_desc.srcStageMask = (VkPipelineStageFlags)(rhi_desc).srcStageMask;
            vk_desc.dstStageMask = (VkPipelineStageFlags)(rhi_desc).dstStageMask;
            vk_desc.srcAccessMask = (VkAccessFlags)(rhi_desc).srcAccessMask;
            vk_desc.dstAccessMask = (VkAccessFlags)(rhi_desc).dstAccessMask;
            vk_desc.dependencyFlags = (VkDependencyFlags)(rhi_desc).dependencyFlags;
        };

        VkRenderPassCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkRenderPassCreateFlags)pCreateInfo->flags;
        create_info.attachmentCount = pCreateInfo->attachmentCount;
        create_info.pAttachments = vk_attachments.data();
        create_info.subpassCount = pCreateInfo->subpassCount;
        create_info.pSubpasses = vk_subpass_description.data();
        create_info.dependencyCount = pCreateInfo->dependencyCount;
        create_info.pDependencies = vk_subpass_depandecy.data();

        pRenderPass = new VulkanRenderPass();
        VkRenderPass vk_render_pass;
        VkResult result = vkCreateRenderPass(mDevice, &create_info, nullptr, &vk_render_pass);
        ((VulkanRenderPass*)pRenderPass)->SetResource(vk_render_pass);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateRenderPass failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler* &pSampler)
    {
        VkSamplerCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkSamplerCreateFlags)pCreateInfo->flags;
        create_info.magFilter = (VkFilter)pCreateInfo->magFilter;
        create_info.minFilter = (VkFilter)pCreateInfo->minFilter;
        create_info.mipmapMode = (VkSamplerMipmapMode)pCreateInfo->mipmapMode;
        create_info.addressModeU = (VkSamplerAddressMode)pCreateInfo->addressModeU;
        create_info.addressModeV = (VkSamplerAddressMode)pCreateInfo->addressModeV;
        create_info.addressModeW = (VkSamplerAddressMode)pCreateInfo->addressModeW;
        create_info.mipLodBias = pCreateInfo->mipLodBias;
        create_info.anisotropyEnable = (VkBool32)pCreateInfo->anisotropyEnable;
        create_info.maxAnisotropy = pCreateInfo->maxAnisotropy;
        create_info.compareEnable = (VkBool32)pCreateInfo->compareEnable;
        create_info.compareOp = (VkCompareOp)pCreateInfo->compareOp;
        create_info.minLod = pCreateInfo->minLod;
        create_info.maxLod = pCreateInfo->maxLod;
        create_info.borderColor = (VkBorderColor)pCreateInfo->borderColor;
        create_info.unnormalizedCoordinates = (VkBool32)pCreateInfo->unnormalizedCoordinates;

        pSampler = new VulkanSampler();
        VkSampler vk_sampler;
        VkResult result = vkCreateSampler(mDevice, &create_info, nullptr, &vk_sampler);
        ((VulkanSampler*)pSampler)->SetResource(vk_sampler);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateSampler failed!");
            return false;
        }
    }

    bool VulkanRHI::CreateSemaphores(const RHISemaphoreCreateInfo* pCreateInfo, RHISemaphore*& pSemaphore)
    {
        VkSemaphoreCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = pCreateInfo->pNext;
        create_info.flags = (VkSemaphoreCreateFlags)pCreateInfo->flags;

        pSemaphore = new VulkanSemaphore();
        VkSemaphore vk_semaphore;
        VkResult result = vkCreateSemaphore(mDevice, &create_info, nullptr, &vk_semaphore);
        ((VulkanSemaphore*)pSemaphore)->SetResource(vk_semaphore);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateSemaphore failed!");
            return false;
        }
    }

    bool VulkanRHI::WaitForFencesPFN(uint32_t fenceCount, RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout)
    {
        //fence
        int fence_size = fenceCount;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = pFences[i];
            auto& vk_fence_element = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->GetResource();
        };

        VkResult result = pfnVkWaitForFences(mDevice, fenceCount, vk_fence_list.data(), waitAll, timeout);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("pfnVkWaitForFences failed!");
            return false;
        }
    }

    bool VulkanRHI::ResetFencesPFN(uint32_t fenceCount, RHIFence* const* pFences)
    {
        //fence
        int fence_size = fenceCount;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = pFences[i];
            auto& vk_fence_element = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->GetResource();
        };

        VkResult result = pfnVkResetFences(mDevice, fenceCount, vk_fence_list.data());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("pfnVkResetFences failed!");
            return false;
        }
    }

    bool VulkanRHI::ResetCommandPoolPFN(RHICommandPool* commandPool, RHICommandPoolResetFlags flags)
    {
        VkResult result = pfnVkResetCommandPool(mDevice, ((VulkanCommandPool*)commandPool)->GetResource(), (VkCommandPoolResetFlags)flags);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("pfnVkResetCommandPool failed!");
            return false;
        }
    }

    bool VulkanRHI::BeginCommandBufferPFN(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        VkCommandBufferInheritanceInfo command_buffer_inheritance_info{};
        if (pBeginInfo->pInheritanceInfo != nullptr)
        {
            command_buffer_inheritance_info.sType = (VkStructureType)pBeginInfo->pInheritanceInfo->sType;
            command_buffer_inheritance_info.pNext = (const void*)pBeginInfo->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass = ((VulkanRenderPass*)pBeginInfo->pInheritanceInfo->renderPass)->GetResource();
            command_buffer_inheritance_info.subpass = pBeginInfo->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer = ((VulkanFrameBuffer*)pBeginInfo->pInheritanceInfo->framebuffer)->GetResource();
            command_buffer_inheritance_info.occlusionQueryEnable = (VkBool32)pBeginInfo->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags = (VkQueryControlFlags)pBeginInfo->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics = (VkQueryPipelineStatisticFlags)pBeginInfo->pInheritanceInfo->pipelineStatistics;

            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = (VkStructureType)pBeginInfo->sType;
        command_buffer_begin_info.pNext = (const void*)pBeginInfo->pNext;
        command_buffer_begin_info.flags = (VkCommandBufferUsageFlags)pBeginInfo->flags;
        command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;
        VkResult result = pfnVkBeginCommandBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource(), &command_buffer_begin_info);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("pfnVkBeginCommandBuffer failed!");
            return false;
        }
    }

    bool VulkanRHI::EndCommandBufferPFN(RHICommandBuffer* commandBuffer)
    {
        VkResult result = pfnVkEndCommandBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("pfnVkEndCommandBuffer failed!");
            return false;
        }
    }

    void VulkanRHI::CmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents)
    {
        VkOffset2D offset_2d{};
        offset_2d.x = pRenderPassBegin->renderArea.offset.x;
        offset_2d.y = pRenderPassBegin->renderArea.offset.y;

        VkExtent2D extent_2d{};
        extent_2d.width = pRenderPassBegin->renderArea.extent.width;
        extent_2d.height = pRenderPassBegin->renderArea.extent.height;

        VkRect2D rect_2d{};
        rect_2d.offset = offset_2d;
        rect_2d.extent = extent_2d;

        //clear_values
        int clear_value_size = pRenderPassBegin->clearValueCount;
        std::vector<VkClearValue> vk_clear_value_list(clear_value_size);
        for (int i = 0; i < clear_value_size; ++i)
        {
            const auto& rhi_clear_value_element = pRenderPassBegin->pClearValues[i];
            auto& vk_clear_value_element = vk_clear_value_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_value_element.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_value_element.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_value_element.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_value_element.color.float32[3];
            vk_clear_color_value.int32[0] = rhi_clear_value_element.color.int32[0];
            vk_clear_color_value.int32[1] = rhi_clear_value_element.color.int32[1];
            vk_clear_color_value.int32[2] = rhi_clear_value_element.color.int32[2];
            vk_clear_color_value.int32[3] = rhi_clear_value_element.color.int32[3];
            vk_clear_color_value.uint32[0] = rhi_clear_value_element.color.uint32[0];
            vk_clear_color_value.uint32[1] = rhi_clear_value_element.color.uint32[1];
            vk_clear_color_value.uint32[2] = rhi_clear_value_element.color.uint32[2];
            vk_clear_color_value.uint32[3] = rhi_clear_value_element.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth = rhi_clear_value_element.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_value_element.depthStencil.stencil;

            vk_clear_value_element.color = vk_clear_color_value;
            vk_clear_value_element.depthStencil = vk_clear_depth_stencil_value;

        };

        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = (VkStructureType)pRenderPassBegin->sType;
        vk_render_pass_begin_info.pNext = pRenderPassBegin->pNext;
        vk_render_pass_begin_info.renderPass = ((VulkanRenderPass*)pRenderPassBegin->renderPass)->GetResource();
        vk_render_pass_begin_info.framebuffer = ((VulkanFrameBuffer*)pRenderPassBegin->framebuffer)->GetResource();
        vk_render_pass_begin_info.renderArea = rect_2d;
        vk_render_pass_begin_info.clearValueCount = pRenderPassBegin->clearValueCount;
        vk_render_pass_begin_info.pClearValues = vk_clear_value_list.data();

        return pfnVkCmdBeginRenderPass(((VulkanCommandBuffer*)commandBuffer)->GetResource(), &vk_render_pass_begin_info, (VkSubpassContents)contents);
    }

    void VulkanRHI::CmdNextSubpassPFN(RHICommandBuffer* commandBuffer, RHISubpassContents contents)
    {
        return pfnVkCmdNextSubpass(((VulkanCommandBuffer*)commandBuffer)->GetResource(), ((VkSubpassContents)contents));
    }

    void VulkanRHI::CmdEndRenderPassPFN(RHICommandBuffer* commandBuffer)
    {
        return pfnVkCmdEndRenderPass(((VulkanCommandBuffer*)commandBuffer)->GetResource());
    }

    void VulkanRHI::CmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline)
    {
        return pfnVkCmdBindPipeline(((VulkanCommandBuffer*)commandBuffer)->GetResource(), (VkPipelineBindPoint)pipelineBindPoint, ((VulkanPipeline*)pipeline)->GetResource());
    }

    void VulkanRHI::CmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports)
    {
        //viewport
        int viewport_size = viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = pViewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];

            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        };

        return pfnVkCmdSetViewport(((VulkanCommandBuffer*)commandBuffer)->GetResource(), firstViewport, viewportCount, vk_viewport_list.data());
    }

    void VulkanRHI::CmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors)
    {
        //rect_2d
        int rect_2d_size = scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = pScissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset_2d{};
            offset_2d.x = rhi_rect_2d_element.offset.x;
            offset_2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extent_2d{};
            extent_2d.width = rhi_rect_2d_element.extent.width;
            extent_2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = (VkOffset2D)offset_2d;
            vk_rect_2d_element.extent = (VkExtent2D)extent_2d;

        };

        return pfnVkCmdSetScissor(((VulkanCommandBuffer*)commandBuffer)->GetResource(), firstScissor, scissorCount, vk_rect_2d_list.data());
    }

    void VulkanRHI::CmdBindVertexBuffersPFN(
        RHICommandBuffer* commandBuffer,
        uint32_t firstBinding,
        uint32_t bindingCount,
        RHIBuffer* const* pBuffers,
        const RHIDeviceSize* pOffsets)
    {
        //buffer
        int buffer_size = bindingCount;
        std::vector<VkBuffer> vk_buffer_list(buffer_size);
        for (int i = 0; i < buffer_size; ++i)
        {
            const auto& rhi_buffer_element = pBuffers[i];
            auto& vk_buffer_element = vk_buffer_list[i];

            vk_buffer_element = ((VulkanBuffer*)rhi_buffer_element)->GetResource();
        };

        //offset
        int offset_size = bindingCount;
        std::vector<VkDeviceSize> vk_device_size_list(offset_size);
        for (int i = 0; i < offset_size; ++i)
        {
            const auto& rhi_offset_element = pOffsets[i];
            auto& vk_offset_element = vk_device_size_list[i];

            vk_offset_element = rhi_offset_element;
        };

        return pfnVkCmdBindVertexBuffers(((VulkanCommandBuffer*)commandBuffer)->GetResource(), firstBinding, bindingCount, vk_buffer_list.data(), vk_device_size_list.data());
    }

    void VulkanRHI::CmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType)
    {
        return pfnVkCmdBindIndexBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource(), ((VulkanBuffer*)buffer)->GetResource(), (VkDeviceSize)offset, (VkIndexType)indexType);
    }

    void VulkanRHI::CmdBindDescriptorSetsPFN(
        RHICommandBuffer* commandBuffer,
        RHIPipelineBindPoint pipelineBindPoint,
        RHIPipelineLayout* layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const RHIDescriptorSet* const* pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets)
    {
        //descriptor_set
        int descriptor_set_size = descriptorSetCount;
        std::vector<VkDescriptorSet> vk_descriptor_set_list(descriptor_set_size);
        for (int i = 0; i < descriptor_set_size; ++i)
        {
            const auto& rhi_descriptor_set_element = pDescriptorSets[i];
            auto& vk_descriptor_set_element = vk_descriptor_set_list[i];

            vk_descriptor_set_element = ((VulkanDescriptorSet*)rhi_descriptor_set_element)->GetResource();
        };

        //offset
        int offset_size = dynamicOffsetCount;
        std::vector<uint32_t> vk_offset_list(offset_size);
        for (int i = 0; i < offset_size; ++i)
        {
            const auto& rhi_offset_element = pDynamicOffsets[i];
            auto& vk_offset_element = vk_offset_list[i];

            vk_offset_element = rhi_offset_element;
        };

        return pfnVkCmdBindDescriptorSets(
            ((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            (VkPipelineBindPoint)pipelineBindPoint,
            ((VulkanPipelineLayout*)layout)->GetResource(),
            firstSet, descriptorSetCount,
            vk_descriptor_set_list.data(),
            dynamicOffsetCount,
            vk_offset_list.data());
    }

    void VulkanRHI::CmdDrawIndexed(RHICommandBuffer* commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        return pfnVkCmdDrawIndexed(((VulkanCommandBuffer*)commandBuffer)->GetResource(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanRHI::CmdClearAttachmentsPFN(
        RHICommandBuffer* commandBuffer,
        uint32_t attachmentCount,
        const RHIClearAttachment* pAttachments,
        uint32_t rectCount,
        const RHIClearRect* pRects)
    {
        //clear_attachment
        int clear_attachment_size = attachmentCount;
        std::vector<VkClearAttachment> vk_clear_attachment_list(clear_attachment_size);
        for (int i = 0; i < clear_attachment_size; ++i)
        {
            const auto& rhi_clear_attachment_element = pAttachments[i];
            auto& vk_clear_attachment_element = vk_clear_attachment_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_attachment_element.clearValue.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_attachment_element.clearValue.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_attachment_element.clearValue.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_attachment_element.clearValue.color.float32[3];
            vk_clear_color_value.int32[0] = rhi_clear_attachment_element.clearValue.color.int32[0];
            vk_clear_color_value.int32[1] = rhi_clear_attachment_element.clearValue.color.int32[1];
            vk_clear_color_value.int32[2] = rhi_clear_attachment_element.clearValue.color.int32[2];
            vk_clear_color_value.int32[3] = rhi_clear_attachment_element.clearValue.color.int32[3];
            vk_clear_color_value.uint32[0] = rhi_clear_attachment_element.clearValue.color.uint32[0];
            vk_clear_color_value.uint32[1] = rhi_clear_attachment_element.clearValue.color.uint32[1];
            vk_clear_color_value.uint32[2] = rhi_clear_attachment_element.clearValue.color.uint32[2];
            vk_clear_color_value.uint32[3] = rhi_clear_attachment_element.clearValue.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth = rhi_clear_attachment_element.clearValue.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_attachment_element.clearValue.depthStencil.stencil;

            vk_clear_attachment_element.clearValue.color = vk_clear_color_value;
            vk_clear_attachment_element.clearValue.depthStencil = vk_clear_depth_stencil_value;
            vk_clear_attachment_element.aspectMask = rhi_clear_attachment_element.aspectMask;
            vk_clear_attachment_element.colorAttachment = rhi_clear_attachment_element.colorAttachment;
        };

        //clear_rect
        int clear_rect_size = rectCount;
        std::vector<VkClearRect> vk_clear_rect_list(clear_rect_size);
        for (int i = 0; i < clear_rect_size; ++i)
        {
            const auto& rhi_clear_rect_element = pRects[i];
            auto& vk_clear_rect_element = vk_clear_rect_list[i];

            VkOffset2D offset_2d{};
            offset_2d.x = rhi_clear_rect_element.rect.offset.x;
            offset_2d.y = rhi_clear_rect_element.rect.offset.y;

            VkExtent2D extent_2d{};
            extent_2d.width = rhi_clear_rect_element.rect.extent.width;
            extent_2d.height = rhi_clear_rect_element.rect.extent.height;

            vk_clear_rect_element.rect.offset = (VkOffset2D)offset_2d;
            vk_clear_rect_element.rect.extent = (VkExtent2D)extent_2d;
            vk_clear_rect_element.baseArrayLayer = rhi_clear_rect_element.baseArrayLayer;
            vk_clear_rect_element.layerCount = rhi_clear_rect_element.layerCount;
        };

        return pfnVkCmdClearAttachments(
            ((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            attachmentCount,
            vk_clear_attachment_list.data(),
            rectCount,
            vk_clear_rect_list.data());
    }

    bool VulkanRHI::BeginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        VkCommandBufferInheritanceInfo command_buffer_inheritance_info{};
        const VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        if (pBeginInfo->pInheritanceInfo != nullptr)
        {
            command_buffer_inheritance_info.sType = (VkStructureType)(pBeginInfo->pInheritanceInfo->sType);
            command_buffer_inheritance_info.pNext = (const void*)pBeginInfo->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass = ((VulkanRenderPass*)pBeginInfo->pInheritanceInfo->renderPass)->GetResource();
            command_buffer_inheritance_info.subpass = pBeginInfo->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer = ((VulkanFrameBuffer*)(pBeginInfo->pInheritanceInfo->framebuffer))->GetResource();
            command_buffer_inheritance_info.occlusionQueryEnable = (VkBool32)pBeginInfo->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags = (VkQueryControlFlags)pBeginInfo->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics = (VkQueryPipelineStatisticFlags)pBeginInfo->pInheritanceInfo->pipelineStatistics;

            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = (VkStructureType)pBeginInfo->sType;
        command_buffer_begin_info.pNext = (const void*)pBeginInfo->pNext;
        command_buffer_begin_info.flags = (VkCommandBufferUsageFlags)pBeginInfo->flags;
        command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;

        VkResult result = vkBeginCommandBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource(), &command_buffer_begin_info);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkBeginCommandBuffer failed!");
            return false;
        }
    }

    bool VulkanRHI::EndCommandBuffer(RHICommandBuffer* commandBuffer)
    {
        VkResult result = vkEndCommandBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkEndCommandBuffer failed!");
            return false;
        }
    }

    void VulkanRHI::UpdateDescriptorSets(
        uint32_t descriptorWriteCount,
        const RHIWriteDescriptorSet* pDescriptorWrites,
        uint32_t descriptorCopyCount,
        const RHICopyDescriptorSet* pDescriptorCopies)
    {
        //write_descriptor_set
        int write_descriptor_set_size = descriptorWriteCount;
        std::vector<VkWriteDescriptorSet> vk_write_descriptor_set_list(write_descriptor_set_size);
        int image_info_count = 0;
        int buffer_info_count = 0;
        for (int i = 0; i < write_descriptor_set_size; ++i)
        {
            const auto& rhi_write_descriptor_set_element = pDescriptorWrites[i];
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr)
            {
                image_info_count++;
            }
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr)
            {
                buffer_info_count++;
            }
        }
        std::vector<VkDescriptorImageInfo> vk_descriptor_image_info_list(image_info_count);
        std::vector<VkDescriptorBufferInfo> vk_descriptor_buffer_info_list(buffer_info_count);
        int image_info_current = 0;
        int buffer_info_current = 0;

        for (int i = 0; i < write_descriptor_set_size; ++i)
        {
            const auto& rhi_write_descriptor_set_element = pDescriptorWrites[i];
            auto& vk_write_descriptor_set_element = vk_write_descriptor_set_list[i];

            const VkDescriptorImageInfo* vk_descriptor_image_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr)
            {
                auto& vk_descriptor_image_info = vk_descriptor_image_info_list[image_info_current];
                if (rhi_write_descriptor_set_element.pImageInfo->sampler == nullptr)
                {
                    vk_descriptor_image_info.sampler = nullptr;
                }
                else
                {
                    vk_descriptor_image_info.sampler = ((VulkanSampler*)rhi_write_descriptor_set_element.pImageInfo->sampler)->GetResource();
                }
                vk_descriptor_image_info.imageView = ((VulkanImageView*)rhi_write_descriptor_set_element.pImageInfo->imageView)->GetResource();
                vk_descriptor_image_info.imageLayout = (VkImageLayout)rhi_write_descriptor_set_element.pImageInfo->imageLayout;

                vk_descriptor_image_info_ptr = &vk_descriptor_image_info;
                image_info_current++;
            }

            const VkDescriptorBufferInfo* vk_descriptor_buffer_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr)
            {
                auto& vk_descriptor_buffer_info = vk_descriptor_buffer_info_list[buffer_info_current];
                vk_descriptor_buffer_info.buffer = ((VulkanBuffer*)rhi_write_descriptor_set_element.pBufferInfo->buffer)->GetResource();
                vk_descriptor_buffer_info.offset = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->offset;
                vk_descriptor_buffer_info.range = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->range;

                vk_descriptor_buffer_info_ptr = &vk_descriptor_buffer_info;
                buffer_info_current++;
            }

            vk_write_descriptor_set_element.sType = (VkStructureType)rhi_write_descriptor_set_element.sType;
            vk_write_descriptor_set_element.pNext = (const void*)rhi_write_descriptor_set_element.pNext;
            vk_write_descriptor_set_element.dstSet = ((VulkanDescriptorSet*)rhi_write_descriptor_set_element.dstSet)->GetResource();
            vk_write_descriptor_set_element.dstBinding = rhi_write_descriptor_set_element.dstBinding;
            vk_write_descriptor_set_element.dstArrayElement = rhi_write_descriptor_set_element.dstArrayElement;
            vk_write_descriptor_set_element.descriptorCount = rhi_write_descriptor_set_element.descriptorCount;
            vk_write_descriptor_set_element.descriptorType = (VkDescriptorType)rhi_write_descriptor_set_element.descriptorType;
            vk_write_descriptor_set_element.pImageInfo = vk_descriptor_image_info_ptr;
            vk_write_descriptor_set_element.pBufferInfo = vk_descriptor_buffer_info_ptr;
            //vk_write_descriptor_set_element.pTexelBufferView = &((VulkanBufferView*)rhi_write_descriptor_set_element.pTexelBufferView)->GetResource();
        };

        if (image_info_current != image_info_count
            || buffer_info_current != buffer_info_count)
        {
            LOG_ERROR("image_info_current != image_info_count || buffer_info_current != buffer_info_count");
            return;
        }

        //copy_descriptor_set
        int copy_descriptor_set_size = descriptorCopyCount;
        std::vector<VkCopyDescriptorSet> vk_copy_descriptor_set_list(copy_descriptor_set_size);
        for (int i = 0; i < copy_descriptor_set_size; ++i)
        {
            const auto& rhi_copy_descriptor_set_element = pDescriptorCopies[i];
            auto& vk_copy_descriptor_set_element = vk_copy_descriptor_set_list[i];

            vk_copy_descriptor_set_element.sType = (VkStructureType)rhi_copy_descriptor_set_element.sType;
            vk_copy_descriptor_set_element.pNext = (const void*)rhi_copy_descriptor_set_element.pNext;
            vk_copy_descriptor_set_element.srcSet = ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.srcSet)->GetResource();
            vk_copy_descriptor_set_element.srcBinding = rhi_copy_descriptor_set_element.srcBinding;
            vk_copy_descriptor_set_element.srcArrayElement = rhi_copy_descriptor_set_element.srcArrayElement;
            vk_copy_descriptor_set_element.dstSet = ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.dstSet)->GetResource();
            vk_copy_descriptor_set_element.dstBinding = rhi_copy_descriptor_set_element.dstBinding;
            vk_copy_descriptor_set_element.dstArrayElement = rhi_copy_descriptor_set_element.dstArrayElement;
            vk_copy_descriptor_set_element.descriptorCount = rhi_copy_descriptor_set_element.descriptorCount;
        };

        vkUpdateDescriptorSets(mDevice, descriptorWriteCount, vk_write_descriptor_set_list.data(), descriptorCopyCount, vk_copy_descriptor_set_list.data());
    }

    bool VulkanRHI::QueueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence)
    {
        //submit_info
        int command_buffer_size_total = 0;
        int semaphore_size_total = 0;
        int signal_semaphore_size_total = 0;
        int pipeline_stage_flags_size_total = 0;

        int submit_info_size = submitCount;
        for (int i = 0; i < submit_info_size; ++i)
        {
            const auto& rhi_submit_info_element = pSubmits[i];
            command_buffer_size_total += rhi_submit_info_element.commandBufferCount;
            semaphore_size_total += rhi_submit_info_element.waitSemaphoreCount;
            signal_semaphore_size_total += rhi_submit_info_element.signalSemaphoreCount;
            pipeline_stage_flags_size_total += rhi_submit_info_element.waitSemaphoreCount;
        }
        std::vector<VkCommandBuffer> vk_command_buffer_list_external(command_buffer_size_total);
        std::vector<VkSemaphore> vk_semaphore_list_external(semaphore_size_total);
        std::vector<VkSemaphore> vk_signal_semaphore_list_external(signal_semaphore_size_total);
        std::vector<VkPipelineStageFlags> vk_pipeline_stage_flags_list_external(pipeline_stage_flags_size_total);

        int command_buffer_size_current = 0;
        int semaphore_size_current = 0;
        int signal_semaphore_size_current = 0;
        int pipeline_stage_flags_size_current = 0;


        std::vector<VkSubmitInfo> vk_submit_info_list(submit_info_size);
        for (int i = 0; i < submit_info_size; ++i)
        {
            const auto& rhi_submit_info_element = pSubmits[i];
            auto& vk_submit_info_element = vk_submit_info_list[i];

            vk_submit_info_element.sType = (VkStructureType)rhi_submit_info_element.sType;
            vk_submit_info_element.pNext = (const void*)rhi_submit_info_element.pNext;

            //command_buffer
            if (rhi_submit_info_element.commandBufferCount > 0)
            {
                vk_submit_info_element.commandBufferCount = rhi_submit_info_element.commandBufferCount;
                vk_submit_info_element.pCommandBuffers = &vk_command_buffer_list_external[command_buffer_size_current];
                int command_buffer_size = rhi_submit_info_element.commandBufferCount;
                for (int i = 0; i < command_buffer_size; ++i)
                {
                    const auto& rhi_command_buffer_element = rhi_submit_info_element.pCommandBuffers[i];
                    auto& vk_command_buffer_element = vk_command_buffer_list_external[command_buffer_size_current];

                    vk_command_buffer_element = ((VulkanCommandBuffer*)rhi_command_buffer_element)->GetResource();

                    command_buffer_size_current++;
                };
            }

            //semaphore
            if (rhi_submit_info_element.waitSemaphoreCount > 0)
            {
                vk_submit_info_element.waitSemaphoreCount = rhi_submit_info_element.waitSemaphoreCount;
                vk_submit_info_element.pWaitSemaphores = &vk_semaphore_list_external[semaphore_size_current];
                int semaphore_size = rhi_submit_info_element.waitSemaphoreCount;
                for (int i = 0; i < semaphore_size; ++i)
                {
                    const auto& rhi_semaphore_element = rhi_submit_info_element.pWaitSemaphores[i];
                    auto& vk_semaphore_element = vk_semaphore_list_external[semaphore_size_current];

                    vk_semaphore_element = ((VulkanSemaphore*)rhi_semaphore_element)->GetResource();

                    semaphore_size_current++;
                };
            }

            //signal_semaphore
            if (rhi_submit_info_element.signalSemaphoreCount > 0)
            {
                vk_submit_info_element.signalSemaphoreCount = rhi_submit_info_element.signalSemaphoreCount;
                vk_submit_info_element.pSignalSemaphores = &vk_signal_semaphore_list_external[signal_semaphore_size_current];
                int signal_semaphore_size = rhi_submit_info_element.signalSemaphoreCount;
                for (int i = 0; i < signal_semaphore_size; ++i)
                {
                    const auto& rhi_signal_semaphore_element = rhi_submit_info_element.pSignalSemaphores[i];
                    auto& vk_signal_semaphore_element = vk_signal_semaphore_list_external[signal_semaphore_size_current];

                    vk_signal_semaphore_element = ((VulkanSemaphore*)rhi_signal_semaphore_element)->GetResource();

                    signal_semaphore_size_current++;
                };
            }

            //pipeline_stage_flags
            if (rhi_submit_info_element.waitSemaphoreCount > 0)
            {
                vk_submit_info_element.pWaitDstStageMask = &vk_pipeline_stage_flags_list_external[pipeline_stage_flags_size_current];
                int pipeline_stage_flags_size = rhi_submit_info_element.waitSemaphoreCount;
                for (int i = 0; i < pipeline_stage_flags_size; ++i)
                {
                    const auto& rhi_pipeline_stage_flags_element = rhi_submit_info_element.pWaitDstStageMask[i];
                    auto& vk_pipeline_stage_flags_element = vk_pipeline_stage_flags_list_external[pipeline_stage_flags_size_current];

                    vk_pipeline_stage_flags_element = (VkPipelineStageFlags)rhi_pipeline_stage_flags_element;

                    pipeline_stage_flags_size_current++;
                };
            }
        };
        

        if ((command_buffer_size_total != command_buffer_size_current)
            || (semaphore_size_total != semaphore_size_current)
            || (signal_semaphore_size_total != signal_semaphore_size_current)
            || (pipeline_stage_flags_size_total != pipeline_stage_flags_size_current))
        {
            LOG_ERROR("submit info is not right!");
            return false;
        }

        VkFence vk_fence = VK_NULL_HANDLE;
        if (fence != nullptr)
        {
            vk_fence = ((VulkanFence*)fence)->GetResource();
        }

        VkResult result = vkQueueSubmit(((VulkanQueue*)queue)->GetResource(), submitCount, vk_submit_info_list.data(), vk_fence);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkQueueSubmit failed!");
            return false;
        }
    }

    bool VulkanRHI::QueueWaitIdle(RHIQueue* queue)
    {
        VkResult result = vkQueueWaitIdle(((VulkanQueue*)queue)->GetResource());

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkQueueWaitIdle failed!");
            return false;
        }
    }

    void VulkanRHI::CmdPipelineBarrier(RHICommandBuffer* commandBuffer,
        RHIPipelineStageFlags srcStageMask,
        RHIPipelineStageFlags dstStageMask,
        RHIDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const RHIMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const RHIBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const RHIImageMemoryBarrier* pImageMemoryBarriers)
    {

        //memory_barrier
        int memory_barrier_size = memoryBarrierCount;
        std::vector<VkMemoryBarrier> vk_memory_barrier_list(memory_barrier_size);
        for (int i = 0; i < memory_barrier_size; ++i)
        {
            const auto& rhi_memory_barrier_element = pMemoryBarriers[i];
            auto& vk_memory_barrier_element = vk_memory_barrier_list[i];


            vk_memory_barrier_element.sType = (VkStructureType)rhi_memory_barrier_element.sType;
            vk_memory_barrier_element.pNext = (const void*)rhi_memory_barrier_element.pNext;
            vk_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_memory_barrier_element.srcAccessMask;
            vk_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_memory_barrier_element.dstAccessMask;
        };

        //buffer_memory_barrier
        int buffer_memory_barrier_size = bufferMemoryBarrierCount;
        std::vector<VkBufferMemoryBarrier> vk_buffer_memory_barrier_list(buffer_memory_barrier_size);
        for (int i = 0; i < buffer_memory_barrier_size; ++i)
        {
            const auto& rhi_buffer_memory_barrier_element = pBufferMemoryBarriers[i];
            auto& vk_buffer_memory_barrier_element = vk_buffer_memory_barrier_list[i];

            vk_buffer_memory_barrier_element.sType = (VkStructureType)rhi_buffer_memory_barrier_element.sType;
            vk_buffer_memory_barrier_element.pNext = (const void*)rhi_buffer_memory_barrier_element.pNext;
            vk_buffer_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_buffer_memory_barrier_element.srcAccessMask;
            vk_buffer_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_buffer_memory_barrier_element.dstAccessMask;
            vk_buffer_memory_barrier_element.srcQueueFamilyIndex = rhi_buffer_memory_barrier_element.srcQueueFamilyIndex;
            vk_buffer_memory_barrier_element.dstQueueFamilyIndex = rhi_buffer_memory_barrier_element.dstQueueFamilyIndex;
            vk_buffer_memory_barrier_element.buffer = ((VulkanBuffer*)rhi_buffer_memory_barrier_element.buffer)->GetResource();
            vk_buffer_memory_barrier_element.offset = (VkDeviceSize)rhi_buffer_memory_barrier_element.offset;
            vk_buffer_memory_barrier_element.size = (VkDeviceSize)rhi_buffer_memory_barrier_element.size;
        };

        //image_memory_barrier
        int image_memory_barrier_size = imageMemoryBarrierCount;
        std::vector<VkImageMemoryBarrier> vk_image_memory_barrier_list(image_memory_barrier_size);
        for (int i = 0; i < image_memory_barrier_size; ++i)
        {
            const auto& rhi_image_memory_barrier_element = pImageMemoryBarriers[i];
            auto& vk_image_memory_barrier_element = vk_image_memory_barrier_list[i];

            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = (VkImageAspectFlags)rhi_image_memory_barrier_element.subresourceRange.aspectMask;
            image_subresource_range.baseMipLevel = rhi_image_memory_barrier_element.subresourceRange.baseMipLevel;
            image_subresource_range.levelCount = rhi_image_memory_barrier_element.subresourceRange.levelCount;
            image_subresource_range.baseArrayLayer = rhi_image_memory_barrier_element.subresourceRange.baseArrayLayer;
            image_subresource_range.layerCount = rhi_image_memory_barrier_element.subresourceRange.layerCount;

            vk_image_memory_barrier_element.sType = (VkStructureType)rhi_image_memory_barrier_element.sType;
            vk_image_memory_barrier_element.pNext = (const void*)rhi_image_memory_barrier_element.pNext;
            vk_image_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_image_memory_barrier_element.srcAccessMask;
            vk_image_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_image_memory_barrier_element.dstAccessMask;
            vk_image_memory_barrier_element.oldLayout = (VkImageLayout)rhi_image_memory_barrier_element.oldLayout;
            vk_image_memory_barrier_element.newLayout = (VkImageLayout)rhi_image_memory_barrier_element.newLayout;
            vk_image_memory_barrier_element.srcQueueFamilyIndex = rhi_image_memory_barrier_element.srcQueueFamilyIndex;
            vk_image_memory_barrier_element.dstQueueFamilyIndex = rhi_image_memory_barrier_element.dstQueueFamilyIndex;
            vk_image_memory_barrier_element.image = ((VulkanImage*)rhi_image_memory_barrier_element.image)->GetResource();
            vk_image_memory_barrier_element.subresourceRange = image_subresource_range;
        };

        vkCmdPipelineBarrier(
            ((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            (RHIPipelineStageFlags)srcStageMask,
            (RHIPipelineStageFlags)dstStageMask,
            (RHIDependencyFlags)dependencyFlags,
            memoryBarrierCount,
            vk_memory_barrier_list.data(),
            bufferMemoryBarrierCount,
            vk_buffer_memory_barrier_list.data(),
            imageMemoryBarrierCount,
            vk_image_memory_barrier_list.data());
    }

    void VulkanRHI::CmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(((VulkanCommandBuffer*)commandBuffer)->GetResource(), vertexCount, instanceCount, firstVertex, firstInstance);
    }
    
    void VulkanRHI::CmdDispatch(RHICommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        vkCmdDispatch(((VulkanCommandBuffer*)commandBuffer)->GetResource(), groupCountX, groupCountY, groupCountZ);
    }

    void VulkanRHI::CmdDispatchIndirect(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset)
    {
        vkCmdDispatchIndirect(((VulkanCommandBuffer*)commandBuffer)->GetResource(), ((VulkanBuffer*)buffer)->GetResource(), offset);
    }

    void VulkanRHI::CmdCopyImageToBuffer(
        RHICommandBuffer* commandBuffer,
        RHIImage* srcImage,
        RHIImageLayout srcImageLayout,
        RHIBuffer* dstBuffer,
        uint32_t regionCount,
        const RHIBufferImageCopy* pRegions)
    {
        //buffer_image_copy
        int buffer_image_copy_size = regionCount;
        std::vector<VkBufferImageCopy> vk_buffer_image_copy_list(buffer_image_copy_size);
        for (int i = 0; i < buffer_image_copy_size; ++i)
        {
            const auto& rhi_buffer_image_copy_element = pRegions[i];
            auto& vk_buffer_image_copy_element = vk_buffer_image_copy_list[i];

            VkImageSubresourceLayers image_subresource_layers{};
            image_subresource_layers.aspectMask = (VkImageAspectFlags)rhi_buffer_image_copy_element.imageSubresource.aspectMask;
            image_subresource_layers.mipLevel = rhi_buffer_image_copy_element.imageSubresource.mipLevel;
            image_subresource_layers.baseArrayLayer = rhi_buffer_image_copy_element.imageSubresource.baseArrayLayer;
            image_subresource_layers.layerCount = rhi_buffer_image_copy_element.imageSubresource.layerCount;

            VkOffset3D offset_3d{};
            offset_3d.x = rhi_buffer_image_copy_element.imageOffset.x;
            offset_3d.y = rhi_buffer_image_copy_element.imageOffset.y;
            offset_3d.z = rhi_buffer_image_copy_element.imageOffset.z;

            VkExtent3D extent_3d{};
            extent_3d.width = rhi_buffer_image_copy_element.imageExtent.width;
            extent_3d.height = rhi_buffer_image_copy_element.imageExtent.height;
            extent_3d.depth = rhi_buffer_image_copy_element.imageExtent.depth;

            VkBufferImageCopy buffer_image_copy{};
            buffer_image_copy.bufferOffset = (VkDeviceSize)rhi_buffer_image_copy_element.bufferOffset;
            buffer_image_copy.bufferRowLength = rhi_buffer_image_copy_element.bufferRowLength;
            buffer_image_copy.bufferImageHeight = rhi_buffer_image_copy_element.bufferImageHeight;
            buffer_image_copy.imageSubresource = image_subresource_layers;
            buffer_image_copy.imageOffset = offset_3d;
            buffer_image_copy.imageExtent = extent_3d;

            vk_buffer_image_copy_element.bufferOffset = (VkDeviceSize)rhi_buffer_image_copy_element.bufferOffset;
            vk_buffer_image_copy_element.bufferRowLength = rhi_buffer_image_copy_element.bufferRowLength;
            vk_buffer_image_copy_element.bufferImageHeight = rhi_buffer_image_copy_element.bufferImageHeight;
            vk_buffer_image_copy_element.imageSubresource = image_subresource_layers;
            vk_buffer_image_copy_element.imageOffset = offset_3d;
            vk_buffer_image_copy_element.imageExtent = extent_3d;
        };

        vkCmdCopyImageToBuffer(
            ((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            ((VulkanImage*)srcImage)->GetResource(),
            (VkImageLayout)srcImageLayout,
            ((VulkanBuffer*)dstBuffer)->GetResource(),
            regionCount,
            vk_buffer_image_copy_list.data());
    }

    void VulkanRHI::CmdCopyImageToImage(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageAspectFlagBits srcFlag, RHIImage* dstImage, RHIImageAspectFlagBits dstFlag, uint32_t width, uint32_t height)
    {
        VkImageCopy imagecopyRegion = {};
        imagecopyRegion.srcSubresource = { (VkImageAspectFlags)srcFlag, 0, 0, 1 };
        imagecopyRegion.srcOffset = { 0, 0, 0 };
        imagecopyRegion.dstSubresource = { (VkImageAspectFlags)dstFlag, 0, 0, 1 };
        imagecopyRegion.dstOffset = { 0, 0, 0 };
        imagecopyRegion.extent = { width, height, 1 };

        vkCmdCopyImage(((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            ((VulkanImage*)srcImage)->GetResource(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            ((VulkanImage*)dstImage)->GetResource(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imagecopyRegion);
    }

    void VulkanRHI::CmdCopyBuffer(RHICommandBuffer* commandBuffer, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32_t regionCount, RHIBufferCopy* pRegions)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = pRegions->srcOffset;
        copyRegion.dstOffset = pRegions->dstOffset;
        copyRegion.size = pRegions->size;

        vkCmdCopyBuffer(((VulkanCommandBuffer*)commandBuffer)->GetResource(),
            ((VulkanBuffer*)srcBuffer)->GetResource(),
            ((VulkanBuffer*)dstBuffer)->GetResource(),
            regionCount,
            &copyRegion);
    }

    void VulkanRHI::createCommandBuffers()
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info {};
        command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1U;

        for (uint32_t i = 0; i < mkMaxFramesInFlight; ++i)
        {
            command_buffer_allocate_info.commandPool = mCommandPools[i];
            VkCommandBuffer vk_command_buffer;
            if (vkAllocateCommandBuffers(mDevice, &command_buffer_allocate_info, &vk_command_buffer) != VK_SUCCESS)
            {
                LOG_ERROR("vk allocate command buffers");
            }
            mVkCommandBuffers[i] = vk_command_buffer;
            mCommandBuffers[i] = new VulkanCommandBuffer();
            ((VulkanCommandBuffer*)mCommandBuffers[i])->SetResource(vk_command_buffer);
        }
    }

    void VulkanRHI::createDescriptorPool()
    {
        // Since DescriptorSet should be treated as asset in Vulkan, DescriptorPool
        // should be big enough, and thus we can sub-allocate DescriptorSet from
        // DescriptorPool merely as we sub-allocate Buffer/Image from DeviceMemory.

        VkDescriptorPoolSize pool_sizes[7];
        pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        pool_sizes[0].descriptorCount = 3 + 2 + 2 + 2 + 1 + 1 + 3 + 3;
        pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = 1 + 1 + 1 * mMaxVertexBlendingMeshCount;
        pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[2].descriptorCount = 1 * mMaxMaterialCount;
        pool_sizes[3].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[3].descriptorCount = 3 + 5 * mMaxMaterialCount + 1 + 1; // ImGui_ImplVulkan_CreateDeviceObjects
        pool_sizes[4].type            = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pool_sizes[4].descriptorCount = 4 + 1 + 1 + 2;
        pool_sizes[5].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[5].descriptorCount = 3;
        pool_sizes[6].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pool_sizes[6].descriptorCount = 1;

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
        pool_info.pPoolSizes    = pool_sizes;
        pool_info.maxSets =
            1 + 1 + 1 + mMaxMaterialCount + mMaxVertexBlendingMeshCount + 1 + 1; // +skybox + axis descriptor set
        pool_info.flags = 0U;

        if (vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mVkDescPool) != VK_SUCCESS)
        {
            LOG_ERROR("create descriptor pool");
        }

        mDescPool = new VulkanDescriptorPool();
        ((VulkanDescriptorPool*)mDescPool)->SetResource(mVkDescPool);
    }

    // semaphore : signal an image is ready for rendering // ready for presentation
    // (m_vulkan_context._swapchain_images --> semaphores, fences)
    void VulkanRHI::createSyncPrimitives()
    {
        VkSemaphoreCreateInfo semaphore_create_info {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_create_info {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // the fence is initialized as signaled

        for (uint32_t i = 0; i < mkMaxFramesInFlight; i++)
        {
            mImageAvailableForTextureCopySemaphores[i] = new VulkanSemaphore();
            if (vkCreateSemaphore(
                    mDevice, &semaphore_create_info, nullptr, &mImageAvailableForRenderSemaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(
                    mDevice, &semaphore_create_info, nullptr, &mImageFinishedForPresentationSemaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(
                    mDevice, &semaphore_create_info, nullptr, &(((VulkanSemaphore*)mImageAvailableForTextureCopySemaphores[i])->GetResource())) !=
                    VK_SUCCESS ||
                vkCreateFence(mDevice, &fence_create_info, nullptr, &mIsFrameInFlightFences[i]) != VK_SUCCESS)
            {
                LOG_ERROR("vk create semaphore & fence");
            }

            mRHIIsFrameInFlightFences[i] = new VulkanFence();
            ((VulkanFence*)mRHIIsFrameInFlightFences[i])->SetResource(mIsFrameInFlightFences[i]);
        }
    }

    void VulkanRHI::CreateFramebufferImageAndView()
    {
        VulkanUtil::CreateImage(mPhysicalDevice,
                                mDevice,
                                mSwapChainExtent.width,
                                mSwapChainExtent.height,
                                (VkFormat)mDepthImageFormat,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                ((VulkanImage*)mDepthImage)->GetResource(),
                                mDepthImageMemory,
                                0,
                                1,
                                1);

        ((VulkanImageView*)mDepthImageView)->SetResource(
            VulkanUtil::CreateImageView(mDevice, ((VulkanImage*)mDepthImage)->GetResource(), (VkFormat)mDepthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1));
    }

    RHISampler* VulkanRHI::GetOrCreateDefaultSampler(RHIDefaultSamplerType type)
    {
        switch (type)
        {
        case MiniEngine::Default_Sampler_Linear:
            if (mLinearSampler == nullptr)
            {
                mLinearSampler = new VulkanSampler();
                ((VulkanSampler*)mLinearSampler)->SetResource(VulkanUtil::GetOrCreateLinearSampler(mPhysicalDevice, mDevice));
            }
            return mLinearSampler;
            break;

        case MiniEngine::Default_Sampler_Nearest:
            if (mNearestSampler == nullptr)
            {
                mNearestSampler = new VulkanSampler();
                ((VulkanSampler*)mNearestSampler)->SetResource(VulkanUtil::GetOrCreateNearestSampler(mPhysicalDevice, mDevice));
            }
            return mNearestSampler;
            break;

        default:
            return nullptr;
            break;
        }
    }

    RHISampler* VulkanRHI::GetOrCreateMipmapSampler(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            LOG_ERROR("width == 0 || height == 0");
            return nullptr;
        }
        RHISampler* sampler;
        uint32_t  mip_levels = floor(log2(std::max(width, height))) + 1;
        auto      find_sampler = mMipSamplerMap.find(mip_levels);
        if (find_sampler != mMipSamplerMap.end())
        {
            return find_sampler->second;
        }
        else
        {
            sampler = new VulkanSampler();

            VkSampler vk_sampler = VulkanUtil::GetOrCreateMipmapSampler(mPhysicalDevice, mDevice, width, height);

            ((VulkanSampler*)sampler)->SetResource(vk_sampler);

            mMipSamplerMap.insert(std::make_pair(mip_levels, sampler));

            return sampler;
        }
    }

    bool VulkanRHI::CreateGraphicsPipeline(RHIPipelineCache *pipelineCache, uint32_t createInfoCnt, const RHIGraphicsPipelineCreateInfo *pCreateInfo, RHIPipeline *&pPipelines)
    {
        //pipeline_shader_stage_create_info
        int pipeline_shader_stage_create_info_size = pCreateInfo->stageCount;
        std::vector<VkPipelineShaderStageCreateInfo> vk_pipeline_shader_stage_create_info_list(pipeline_shader_stage_create_info_size);

        int specialization_map_entry_size_total = 0;
        int specialization_info_total = 0;
        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                specialization_info_total++;
                specialization_map_entry_size_total+= rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
            }
        }
        std::vector<VkSpecializationInfo> vk_specialization_info_list(specialization_info_total);
        std::vector<VkSpecializationMapEntry> vk_specialization_map_entry_list(specialization_map_entry_size_total);
        int specialization_map_entry_current = 0;
        int specialization_info_current = 0;

        for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i)
        {
            const auto& rhi_pipeline_shader_stage_create_info_element = pCreateInfo->pStages[i];
            auto& vk_pipeline_shader_stage_create_info_element = vk_pipeline_shader_stage_create_info_list[i];

            if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr)
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = &vk_specialization_info_list[specialization_info_current];

                VkSpecializationInfo vk_specialization_info{};
                vk_specialization_info.mapEntryCount = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                vk_specialization_info.pMapEntries = &vk_specialization_map_entry_list[specialization_map_entry_current];
                vk_specialization_info.dataSize = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->dataSize;
                vk_specialization_info.pData = (const void*)rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pData;

                //specialization_map_entry
                for (int i = 0; i < rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount; ++i)
                {
                    const auto& rhi_specialization_map_entry_element = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pMapEntries[i];
                    auto& vk_specialization_map_entry_element = vk_specialization_map_entry_list[specialization_map_entry_current];

                    vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element->constantID;
                    vk_specialization_map_entry_element.offset = rhi_specialization_map_entry_element->offset;
                    vk_specialization_map_entry_element.size = rhi_specialization_map_entry_element->size;

                    specialization_map_entry_current++;
                };

                specialization_info_current++;
            }
            else
            {
                vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = nullptr;
            }
            vk_pipeline_shader_stage_create_info_element.sType = (VkStructureType)rhi_pipeline_shader_stage_create_info_element.sType;
            vk_pipeline_shader_stage_create_info_element.pNext = (const void*)rhi_pipeline_shader_stage_create_info_element.pNext;
            vk_pipeline_shader_stage_create_info_element.flags = (VkPipelineShaderStageCreateFlags)rhi_pipeline_shader_stage_create_info_element.flags;
            vk_pipeline_shader_stage_create_info_element.stage = (VkShaderStageFlagBits)rhi_pipeline_shader_stage_create_info_element.stage;
            vk_pipeline_shader_stage_create_info_element.module = ((VulkanShader*)rhi_pipeline_shader_stage_create_info_element.module)->GetResource();
            vk_pipeline_shader_stage_create_info_element.pName = rhi_pipeline_shader_stage_create_info_element.pName;
        };

        if (!((specialization_map_entry_size_total == specialization_map_entry_current)
            && (specialization_info_total == specialization_info_current)))
        {
            LOG_ERROR("(specialization_map_entry_size_total == specialization_map_entry_current)&& (specialization_info_total == specialization_info_current)");
            return false;
        }

        //vertex_input_binding_description
        int vertex_input_binding_description_size = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        std::vector<VkVertexInputBindingDescription> vk_vertex_input_binding_description_list(vertex_input_binding_description_size);
        for (int i = 0; i < vertex_input_binding_description_size; ++i)
        {
            const auto& rhi_vertex_input_binding_description_element = pCreateInfo->pVertexInputState->pVertexBindingDescriptions[i];
            auto& vk_vertex_input_binding_description_element = vk_vertex_input_binding_description_list[i];

            vk_vertex_input_binding_description_element.binding = rhi_vertex_input_binding_description_element.binding;
            vk_vertex_input_binding_description_element.stride = rhi_vertex_input_binding_description_element.stride;
            vk_vertex_input_binding_description_element.inputRate = (VkVertexInputRate)rhi_vertex_input_binding_description_element.inputRate;
        };

        //vertex_input_attribute_description
        int vertex_input_attribute_description_size = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        std::vector<VkVertexInputAttributeDescription> vk_vertex_input_attribute_description_list(vertex_input_attribute_description_size);
        for (int i = 0; i < vertex_input_attribute_description_size; ++i)
        {
            const auto& rhi_vertex_input_attribute_description_element = pCreateInfo->pVertexInputState->pVertexAttributeDescriptions[i];
            auto& vk_vertex_input_attribute_description_element = vk_vertex_input_attribute_description_list[i];

            vk_vertex_input_attribute_description_element.location = rhi_vertex_input_attribute_description_element.location;
            vk_vertex_input_attribute_description_element.binding = rhi_vertex_input_attribute_description_element.binding;
            vk_vertex_input_attribute_description_element.format = (VkFormat)rhi_vertex_input_attribute_description_element.format;
            vk_vertex_input_attribute_description_element.offset = rhi_vertex_input_attribute_description_element.offset;
        };

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        vk_pipeline_vertex_input_state_create_info.sType = (VkStructureType)pCreateInfo->pVertexInputState->sType;
        vk_pipeline_vertex_input_state_create_info.pNext = (const void*)pCreateInfo->pVertexInputState->pNext;
        vk_pipeline_vertex_input_state_create_info.flags = (VkPipelineVertexInputStateCreateFlags)pCreateInfo->pVertexInputState->flags;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vk_vertex_input_binding_description_list.data();
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_vertex_input_attribute_description_list.data();

        VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{};
        vk_pipeline_input_assembly_state_create_info.sType = (VkStructureType)pCreateInfo->pInputAssemblyState->sType;
        vk_pipeline_input_assembly_state_create_info.pNext = (const void*)pCreateInfo->pInputAssemblyState->pNext;
        vk_pipeline_input_assembly_state_create_info.flags = (VkPipelineInputAssemblyStateCreateFlags)pCreateInfo->pInputAssemblyState->flags;
        vk_pipeline_input_assembly_state_create_info.topology = (VkPrimitiveTopology)pCreateInfo->pInputAssemblyState->topology;
        vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = (VkBool32)pCreateInfo->pInputAssemblyState->primitiveRestartEnable;

        const VkPipelineTessellationStateCreateInfo* vk_pipeline_tessellation_state_create_info_ptr = nullptr;
        VkPipelineTessellationStateCreateInfo vk_pipeline_tessellation_state_create_info{};
        if (pCreateInfo->pTessellationState != nullptr)
        {
            vk_pipeline_tessellation_state_create_info.sType = (VkStructureType)pCreateInfo->pTessellationState->sType;
            vk_pipeline_tessellation_state_create_info.pNext = (const void*)pCreateInfo->pTessellationState->pNext;
            vk_pipeline_tessellation_state_create_info.flags = (VkPipelineTessellationStateCreateFlags)pCreateInfo->pTessellationState->flags;
            vk_pipeline_tessellation_state_create_info.patchControlPoints = pCreateInfo->pTessellationState->patchControlPoints;

            vk_pipeline_tessellation_state_create_info_ptr = &vk_pipeline_tessellation_state_create_info;
        }

        //viewport
        int viewport_size = pCreateInfo->pViewportState->viewportCount;
        std::vector<VkViewport> vk_viewport_list(viewport_size);
        for (int i = 0; i < viewport_size; ++i)
        {
            const auto& rhi_viewport_element = pCreateInfo->pViewportState->pViewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];

            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        };

        //rect_2d
        int rect_2d_size = pCreateInfo->pViewportState->scissorCount;
        std::vector<VkRect2D> vk_rect_2d_list(rect_2d_size);
        for (int i = 0; i < rect_2d_size; ++i)
        {
            const auto& rhi_rect_2d_element = pCreateInfo->pViewportState->pScissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset2d{};
            offset2d.x = rhi_rect_2d_element.offset.x;
            offset2d.y = rhi_rect_2d_element.offset.y;

            VkExtent2D extend2d{};
            extend2d.width = rhi_rect_2d_element.extent.width;
            extend2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = offset2d;
            vk_rect_2d_element.extent = extend2d;
        };

        VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{};
        vk_pipeline_viewport_state_create_info.sType = (VkStructureType)pCreateInfo->pViewportState->sType;
        vk_pipeline_viewport_state_create_info.pNext = (const void*)pCreateInfo->pViewportState->pNext;
        vk_pipeline_viewport_state_create_info.flags = (VkPipelineViewportStateCreateFlags)pCreateInfo->pViewportState->flags;
        vk_pipeline_viewport_state_create_info.viewportCount = pCreateInfo->pViewportState->viewportCount;
        vk_pipeline_viewport_state_create_info.pViewports = vk_viewport_list.data();
        vk_pipeline_viewport_state_create_info.scissorCount = pCreateInfo->pViewportState->scissorCount;
        vk_pipeline_viewport_state_create_info.pScissors = vk_rect_2d_list.data();

        VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{};
        vk_pipeline_rasterization_state_create_info.sType = (VkStructureType)pCreateInfo->pRasterizationState->sType;
        vk_pipeline_rasterization_state_create_info.pNext = (const void*)pCreateInfo->pRasterizationState->pNext;
        vk_pipeline_rasterization_state_create_info.flags = (VkPipelineRasterizationStateCreateFlags)pCreateInfo->pRasterizationState->flags;
        vk_pipeline_rasterization_state_create_info.depthClampEnable = (VkBool32)pCreateInfo->pRasterizationState->depthClampEnable;
        vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable = (VkBool32)pCreateInfo->pRasterizationState->rasterizerDiscardEnable;
        vk_pipeline_rasterization_state_create_info.polygonMode = (VkPolygonMode)pCreateInfo->pRasterizationState->polygonMode;
        vk_pipeline_rasterization_state_create_info.cullMode = (VkCullModeFlags)pCreateInfo->pRasterizationState->cullMode;
        vk_pipeline_rasterization_state_create_info.frontFace = (VkFrontFace)pCreateInfo->pRasterizationState->frontFace;
        vk_pipeline_rasterization_state_create_info.depthBiasEnable = (VkBool32)pCreateInfo->pRasterizationState->depthBiasEnable;
        vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor = pCreateInfo->pRasterizationState->depthBiasConstantFactor;
        vk_pipeline_rasterization_state_create_info.depthBiasClamp = pCreateInfo->pRasterizationState->depthBiasClamp;
        vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor = pCreateInfo->pRasterizationState->depthBiasSlopeFactor;
        vk_pipeline_rasterization_state_create_info.lineWidth = pCreateInfo->pRasterizationState->lineWidth;

        VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};
        vk_pipeline_multisample_state_create_info.sType = (VkStructureType)pCreateInfo->pMultisampleState->sType;
        vk_pipeline_multisample_state_create_info.pNext = (const void*)pCreateInfo->pMultisampleState->pNext;
        vk_pipeline_multisample_state_create_info.flags = (VkPipelineMultisampleStateCreateFlags)pCreateInfo->pMultisampleState->flags;
        vk_pipeline_multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)pCreateInfo->pMultisampleState->rasterizationSamples;
        vk_pipeline_multisample_state_create_info.sampleShadingEnable = (VkBool32)pCreateInfo->pMultisampleState->sampleShadingEnable;
        vk_pipeline_multisample_state_create_info.minSampleShading = pCreateInfo->pMultisampleState->minSampleShading;
        vk_pipeline_multisample_state_create_info.pSampleMask = (const RHISampleMask*)pCreateInfo->pMultisampleState->pSampleMask;
        vk_pipeline_multisample_state_create_info.alphaToCoverageEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToCoverageEnable;
        vk_pipeline_multisample_state_create_info.alphaToOneEnable = (VkBool32)pCreateInfo->pMultisampleState->alphaToOneEnable;

        VkStencilOpState stencil_op_state_front{};
        stencil_op_state_front.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.failOp;
        stencil_op_state_front.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.passOp;
        stencil_op_state_front.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->front.depthFailOp;
        stencil_op_state_front.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->front.compareOp;
        stencil_op_state_front.compareMask = pCreateInfo->pDepthStencilState->front.compareMask;
        stencil_op_state_front.writeMask = pCreateInfo->pDepthStencilState->front.writeMask;
        stencil_op_state_front.reference = pCreateInfo->pDepthStencilState->front.reference;

        VkStencilOpState stencil_op_state_back{};
        stencil_op_state_back.failOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.failOp;
        stencil_op_state_back.passOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.passOp;
        stencil_op_state_back.depthFailOp = (VkStencilOp)pCreateInfo->pDepthStencilState->back.depthFailOp;
        stencil_op_state_back.compareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->back.compareOp;
        stencil_op_state_back.compareMask = pCreateInfo->pDepthStencilState->back.compareMask;
        stencil_op_state_back.writeMask = pCreateInfo->pDepthStencilState->back.writeMask;
        stencil_op_state_back.reference = pCreateInfo->pDepthStencilState->back.reference;


        VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{};
        vk_pipeline_depth_stencil_state_create_info.sType = (VkStructureType)pCreateInfo->pDepthStencilState->sType;
        vk_pipeline_depth_stencil_state_create_info.pNext = (const void*)pCreateInfo->pDepthStencilState->pNext;
        vk_pipeline_depth_stencil_state_create_info.flags = (VkPipelineDepthStencilStateCreateFlags)pCreateInfo->pDepthStencilState->flags;
        vk_pipeline_depth_stencil_state_create_info.depthTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthTestEnable;
        vk_pipeline_depth_stencil_state_create_info.depthWriteEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthWriteEnable;
        vk_pipeline_depth_stencil_state_create_info.depthCompareOp = (VkCompareOp)pCreateInfo->pDepthStencilState->depthCompareOp;
        vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->depthBoundsTestEnable;
        vk_pipeline_depth_stencil_state_create_info.stencilTestEnable = (VkBool32)pCreateInfo->pDepthStencilState->stencilTestEnable;
        vk_pipeline_depth_stencil_state_create_info.front = stencil_op_state_front;
        vk_pipeline_depth_stencil_state_create_info.back = stencil_op_state_back;
        vk_pipeline_depth_stencil_state_create_info.minDepthBounds = pCreateInfo->pDepthStencilState->minDepthBounds;
        vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = pCreateInfo->pDepthStencilState->maxDepthBounds;

        //pipeline_color_blend_attachment_state
        int pipeline_color_blend_attachment_state_size = pCreateInfo->pColorBlendState->attachmentCount;
        std::vector<VkPipelineColorBlendAttachmentState> vk_pipeline_color_blend_attachment_state_list(pipeline_color_blend_attachment_state_size);
        for (int i = 0; i < pipeline_color_blend_attachment_state_size; ++i)
        {
            const auto& rhi_pipeline_color_blend_attachment_state_element = pCreateInfo->pColorBlendState->pAttachments[i];
            auto& vk_pipeline_color_blend_attachment_state_element = vk_pipeline_color_blend_attachment_state_list[i];

            vk_pipeline_color_blend_attachment_state_element.blendEnable = (VkBool32)rhi_pipeline_color_blend_attachment_state_element.blendEnable;
            vk_pipeline_color_blend_attachment_state_element.srcColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstColorBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.colorBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.colorBlendOp;
            vk_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor;
            vk_pipeline_color_blend_attachment_state_element.alphaBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.alphaBlendOp;
            vk_pipeline_color_blend_attachment_state_element.colorWriteMask = (VkColorComponentFlags)rhi_pipeline_color_blend_attachment_state_element.colorWriteMask;
        };

        VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{};
        vk_pipeline_color_blend_state_create_info.sType = (VkStructureType)pCreateInfo->pColorBlendState->sType;
        vk_pipeline_color_blend_state_create_info.pNext = pCreateInfo->pColorBlendState->pNext;
        vk_pipeline_color_blend_state_create_info.flags = pCreateInfo->pColorBlendState->flags;
        vk_pipeline_color_blend_state_create_info.logicOpEnable = pCreateInfo->pColorBlendState->logicOpEnable;
        vk_pipeline_color_blend_state_create_info.logicOp = (VkLogicOp)pCreateInfo->pColorBlendState->logicOp;
        vk_pipeline_color_blend_state_create_info.attachmentCount = pCreateInfo->pColorBlendState->attachmentCount;
        vk_pipeline_color_blend_state_create_info.pAttachments = vk_pipeline_color_blend_attachment_state_list.data();
        for (int i = 0; i < 4; ++i)
        {
            vk_pipeline_color_blend_state_create_info.blendConstants[i] = pCreateInfo->pColorBlendState->blendConstants[i];
        };

        //dynamic_state
        int dynamic_state_size = pCreateInfo->pDynamicState->dynamicStateCount;
        std::vector<VkDynamicState> vk_dynamic_state_list(dynamic_state_size);
        for (int i = 0; i < dynamic_state_size; ++i)
        {
            const auto& rhi_dynamic_state_element = pCreateInfo->pDynamicState->pDynamicStates[i];
            auto& vk_dynamic_state_element = vk_dynamic_state_list[i];

            vk_dynamic_state_element = (VkDynamicState)rhi_dynamic_state_element;
        };

        VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info{};
        vk_pipeline_dynamic_state_create_info.sType = (VkStructureType)pCreateInfo->pDynamicState->sType;
        vk_pipeline_dynamic_state_create_info.pNext = pCreateInfo->pDynamicState->pNext;
        vk_pipeline_dynamic_state_create_info.flags = (VkPipelineDynamicStateCreateFlags)pCreateInfo->pDynamicState->flags;
        vk_pipeline_dynamic_state_create_info.dynamicStateCount = pCreateInfo->pDynamicState->dynamicStateCount;
        vk_pipeline_dynamic_state_create_info.pDynamicStates = vk_dynamic_state_list.data();

        VkGraphicsPipelineCreateInfo create_info{};
        create_info.sType = (VkStructureType)pCreateInfo->sType;
        create_info.pNext = (const void*)pCreateInfo->pNext;
        create_info.flags = (VkPipelineCreateFlags)pCreateInfo->flags;
        create_info.stageCount = pCreateInfo->stageCount;
        create_info.pStages = vk_pipeline_shader_stage_create_info_list.data();
        create_info.pVertexInputState = &vk_pipeline_vertex_input_state_create_info;
        create_info.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
        create_info.pTessellationState = vk_pipeline_tessellation_state_create_info_ptr;
        create_info.pViewportState = &vk_pipeline_viewport_state_create_info;
        create_info.pRasterizationState = &vk_pipeline_rasterization_state_create_info;
        create_info.pMultisampleState = &vk_pipeline_multisample_state_create_info;
        create_info.pDepthStencilState = &vk_pipeline_depth_stencil_state_create_info;
        create_info.pColorBlendState = &vk_pipeline_color_blend_state_create_info;
        create_info.pDynamicState = &vk_pipeline_dynamic_state_create_info;
        create_info.layout = ((VulkanPipelineLayout*)pCreateInfo->layout)->GetResource();
        create_info.renderPass = ((VulkanRenderPass*)pCreateInfo->renderPass)->GetResource();
        create_info.subpass = pCreateInfo->subpass;
        if (pCreateInfo->basePipelineHandle != nullptr)
        {
            create_info.basePipelineHandle = ((VulkanPipeline*)pCreateInfo->basePipelineHandle)->GetResource();
        }
        else
        {
            create_info.basePipelineHandle = VK_NULL_HANDLE;
        }
        create_info.basePipelineIndex = pCreateInfo->basePipelineIndex;

        pPipelines = new VulkanPipeline();
        VkPipeline vk_pipelines;
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipelineCache != nullptr)
        {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipelineCache)->GetResource();
        }
        VkResult result = vkCreateGraphicsPipelines(mDevice, vk_pipeline_cache, createInfoCnt, &create_info, nullptr, &vk_pipelines);
        ((VulkanPipeline*)pPipelines)->SetResource(vk_pipelines);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("vkCreateGraphicsPipelines failed!");
            return false;
        }
    }

    RHIShader* VulkanRHI::CreateShaderModule(const std::vector<unsigned char>& shader_code)
    {
        RHIShader* shahder = new VulkanShader();

        VkShaderModule vk_shader =  VulkanUtil::CreateShaderModule(mDevice, shader_code);

        ((VulkanShader*)shahder)->SetResource(vk_shader);

        return shahder;
    }

    void VulkanRHI::CreateBuffer(RHIDeviceSize size, RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer* & buffer, RHIDeviceMemory* & buffer_memory)
    {
        VkBuffer vk_buffer;
        VkDeviceMemory vk_device_memory;
        
        VulkanUtil::CreateBuffer(mPhysicalDevice, mDevice, size, usage, properties, vk_buffer, vk_device_memory);

        buffer = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();
        ((VulkanBuffer*)buffer)->SetResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer_memory)->SetResource(vk_device_memory);
    }

    void VulkanRHI::CreateBufferAndInitialize(RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory, RHIDeviceSize size, void* data, int datasize)
    {
        VkBuffer vk_buffer;
        VkDeviceMemory vk_device_memory;

        VulkanUtil::CreateBufferAndInitialize(mDevice, mPhysicalDevice, usage, properties, &vk_buffer, &vk_device_memory, size, data, datasize);

        buffer = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();
        ((VulkanBuffer*)buffer)->SetResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer_memory)->SetResource(vk_device_memory);
    }

    bool VulkanRHI::CreateBufferVMA(VmaAllocator allocator, const RHIBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, RHIBuffer* & pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
    {
        VkBuffer vk_buffer;
        VkBufferCreateInfo buffer_create_info{};
        buffer_create_info.sType = (VkStructureType)pBufferCreateInfo->sType;
        buffer_create_info.pNext = (const void*)pBufferCreateInfo->pNext;
        buffer_create_info.flags = (VkBufferCreateFlags)pBufferCreateInfo->flags;
        buffer_create_info.size = (VkDeviceSize)pBufferCreateInfo->size;
        buffer_create_info.usage = (VkBufferUsageFlags)pBufferCreateInfo->usage;
        buffer_create_info.sharingMode = (VkSharingMode)pBufferCreateInfo->sharingMode;
        buffer_create_info.queueFamilyIndexCount = pBufferCreateInfo->queueFamilyIndexCount;
        buffer_create_info.pQueueFamilyIndices = (const uint32_t*)pBufferCreateInfo->pQueueFamilyIndices;

        pBuffer = new VulkanBuffer();
        VkResult result = vmaCreateBuffer(allocator,
            &buffer_create_info,
            pAllocationCreateInfo,
            &vk_buffer,
            pAllocation,
            pAllocationInfo);

        ((VulkanBuffer*)pBuffer)->SetResource(vk_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool VulkanRHI::CreateBufferWithAlignmentVMA(VmaAllocator allocator, const RHIBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, RHIDeviceSize minAlignment, RHIBuffer* &pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
    {
        VkBuffer vk_buffer;
        VkBufferCreateInfo buffer_create_info{};
        buffer_create_info.sType = (VkStructureType)pBufferCreateInfo->sType;
        buffer_create_info.pNext = (const void*)pBufferCreateInfo->pNext;
        buffer_create_info.flags = (VkBufferCreateFlags)pBufferCreateInfo->flags;
        buffer_create_info.size = (VkDeviceSize)pBufferCreateInfo->size;
        buffer_create_info.usage = (VkBufferUsageFlags)pBufferCreateInfo->usage;
        buffer_create_info.sharingMode = (VkSharingMode)pBufferCreateInfo->sharingMode;
        buffer_create_info.queueFamilyIndexCount = pBufferCreateInfo->queueFamilyIndexCount;
        buffer_create_info.pQueueFamilyIndices = (const uint32_t*)pBufferCreateInfo->pQueueFamilyIndices;

        pBuffer = new VulkanBuffer();
        VkResult result = vmaCreateBufferWithAlignment(allocator,
            &buffer_create_info,
            pAllocationCreateInfo,
            minAlignment,
            &vk_buffer,
            pAllocation,
            pAllocationInfo);

        ((VulkanBuffer*)pBuffer)->SetResource(vk_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vmaCreateBufferWithAlignment failed!");
            return false;
        }
    }


    void VulkanRHI::CopyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, RHIDeviceSize srcOffset, RHIDeviceSize dstOffset, RHIDeviceSize size)
    {
        VkBuffer vk_src_buffer = ((VulkanBuffer*)srcBuffer)->GetResource();
        VkBuffer vk_dst_buffer = ((VulkanBuffer*)dstBuffer)->GetResource();
        VulkanUtil::CopyBuffer(this, vk_src_buffer, vk_dst_buffer, srcOffset, dstOffset, size);
    }

    void VulkanRHI::CreateImage(uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
        RHIImage* &image, RHIDeviceMemory* &memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels)
    {
        VkImage vk_image;
        VkDeviceMemory vk_device_memory;
        VulkanUtil::CreateImage(
            mPhysicalDevice,
            mDevice,
            image_width,
            image_height,
            (VkFormat)format,
            (VkImageTiling)image_tiling,
            (VkImageUsageFlags)image_usage_flags,
            (VkMemoryPropertyFlags)memory_property_flags,
            vk_image,
            vk_device_memory,
            (VkImageCreateFlags)image_create_flags,
            array_layers,
            miplevels);

        image = new VulkanImage();
        memory = new VulkanDeviceMemory();
        ((VulkanImage*)image)->SetResource(vk_image);
        ((VulkanDeviceMemory*)memory)->SetResource(vk_device_memory);
    }

    void VulkanRHI::CreateImageView(RHIImage* image, RHIFormat format, RHIImageAspectFlags image_aspect_flags, RHIImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
        RHIImageView* &image_view)
    {
        image_view = new VulkanImageView();
        VkImage vk_image = ((VulkanImage*)image)->GetResource();
        VkImageView vk_image_view;
        vk_image_view = VulkanUtil::CreateImageView(mDevice, vk_image, (VkFormat)format, image_aspect_flags, (VkImageViewType)view_type, layout_count, miplevels);
        ((VulkanImageView*)image_view)->SetResource(vk_image_view);
    }

    void VulkanRHI::CreateGlobalImage(RHIImage* &image, RHIImageView* &image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, void* texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels)
    {
        VkImage vk_image;
        VkImageView vk_image_view;
        
        VulkanUtil::CreateGlobalImage(this, vk_image, vk_image_view,image_allocation,texture_image_width,texture_image_height,texture_image_pixels,texture_image_format,miplevels);
        
        image = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->SetResource(vk_image);
        ((VulkanImageView*)image_view)->SetResource(vk_image_view);
    }

    void VulkanRHI::CreateCubeMap(RHIImage* &image, RHIImageView* &image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, std::array<void*, 6> texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels)
    {
        VkImage vk_image;
        VkImageView vk_image_view;

        VulkanUtil::CreateCubeMap(this, vk_image, vk_image_view, image_allocation, texture_image_width, texture_image_height, texture_image_pixels, texture_image_format, miplevels);

        image = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->SetResource(vk_image);
        ((VulkanImageView*)image_view)->SetResource(vk_image_view);
    }

    void VulkanRHI::CreateSwapChainImageViews()
    {
        mSwapChainImageViews.resize(mSwapChainImages.size());

        // create imageview (one for each this time) for all swapchain images
        for (size_t i = 0; i < mSwapChainImages.size(); i++)
        {
            VkImageView vk_image_view;
            vk_image_view = VulkanUtil::CreateImageView(mDevice,
                                                                   mSwapChainImages[i],
                                                                   (VkFormat)mSwapChainImageFormat,
                                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                                   VK_IMAGE_VIEW_TYPE_2D,
                                                                   1,
                                                                   1);
            mSwapChainImageViews[i] = new VulkanImageView();
            ((VulkanImageView*)mSwapChainImageViews[i])->SetResource(vk_image_view);
        }
    }

    void VulkanRHI::createAssetAllocator()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = mVulkanAPIVersion;
        allocatorCreateInfo.physicalDevice         = mPhysicalDevice;
        allocatorCreateInfo.device                 = mDevice;
        allocatorCreateInfo.instance               = mInstance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &mAssetsAllocator);
    }

    // todo : more descriptorSet
    bool VulkanRHI::AllocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet* &pDescriptorSets)
    {
        //descriptor_set_layout
        int descriptor_set_layout_size = pAllocateInfo->descriptorSetCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);
        for (int i = 0; i < descriptor_set_layout_size; ++i)
        {
            const auto& rhi_descriptor_set_layout_element = pAllocateInfo->pSetLayouts[i];
            auto& vk_descriptor_set_layout_element = vk_descriptor_set_layout_list[i];

            vk_descriptor_set_layout_element = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->GetResource();

            VulkanDescriptorSetLayout* test = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element);

            test = nullptr;
        };

        VkDescriptorSetAllocateInfo descriptorset_allocate_info{};
        descriptorset_allocate_info.sType = (VkStructureType)pAllocateInfo->sType;
        descriptorset_allocate_info.pNext = (const void*)pAllocateInfo->pNext;
        descriptorset_allocate_info.descriptorPool = ((VulkanDescriptorPool*)(pAllocateInfo->descriptorPool))->GetResource();
        descriptorset_allocate_info.descriptorSetCount = pAllocateInfo->descriptorSetCount;
        descriptorset_allocate_info.pSetLayouts = vk_descriptor_set_layout_list.data();

        VkDescriptorSet vk_descriptor_set;
        pDescriptorSets = new VulkanDescriptorSet;
        VkResult result = vkAllocateDescriptorSets(mDevice, &descriptorset_allocate_info, &vk_descriptor_set);
        ((VulkanDescriptorSet*)pDescriptorSets)->SetResource(vk_descriptor_set);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkAllocateDescriptorSets failed!");
            return false;
        }
    }

    bool VulkanRHI::AllocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer* &pCommandBuffers)
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = (VkStructureType)pAllocateInfo->sType;
        command_buffer_allocate_info.pNext = (const void*)pAllocateInfo->pNext;
        command_buffer_allocate_info.commandPool = ((VulkanCommandPool*)(pAllocateInfo->commandPool))->GetResource();
        command_buffer_allocate_info.level = (VkCommandBufferLevel)pAllocateInfo->level;
        command_buffer_allocate_info.commandBufferCount = pAllocateInfo->commandBufferCount;

        VkCommandBuffer vk_command_buffer;
        pCommandBuffers = new RHICommandBuffer();
        VkResult result = vkAllocateCommandBuffers(mDevice, &command_buffer_allocate_info, &vk_command_buffer);
        ((VulkanCommandBuffer*)pCommandBuffers)->SetResource(vk_command_buffer);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkAllocateCommandBuffers failed!");
            return false;
        }
    }

    void VulkanRHI::CreateSwapChain()
    {
        // query all supports of this physical device
        SwapChainSupportDetails swapchain_support_details = querySwapChainSupport(mPhysicalDevice);

        // choose the best or fitting format
        VkSurfaceFormatKHR chosen_surface_format =
            chooseSwapChainSurfaceFormatFromDetails(swapchain_support_details.formats);
        // choose the best or fitting present mode
        VkPresentModeKHR chosen_presentMode =
            chooseSwapChainPresentModeFromDetails(swapchain_support_details.presentModes);
        // choose the best or fitting extent
        VkExtent2D chosen_extent = chooseSwapChainExtentFromDetails(swapchain_support_details.capabilities);

        uint32_t image_count = swapchain_support_details.capabilities.minImageCount + 1;
        if (swapchain_support_details.capabilities.maxImageCount > 0 &&
            image_count > swapchain_support_details.capabilities.maxImageCount)
        {
            image_count = swapchain_support_details.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = mSurface;

        createInfo.minImageCount    = image_count;
        createInfo.imageFormat      = chosen_surface_format.format;
        createInfo.imageColorSpace  = chosen_surface_format.colorSpace;
        createInfo.imageExtent      = chosen_extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {mQueueIndices.graphicsFamily.value(), mQueueIndices.presentFamily.value()};

        if (mQueueIndices.graphicsFamily != mQueueIndices.presentFamily)
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform   = swapchain_support_details.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode    = chosen_presentMode;
        createInfo.clipped        = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
        {
            LOG_ERROR("vk create swapchain khr");
        }

        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &image_count, nullptr);
        mSwapChainImages.resize(image_count);
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &image_count, mSwapChainImages.data());

        mSwapChainImageFormat = (RHIFormat)chosen_surface_format.format;
        mSwapChainExtent.height = chosen_extent.height;
        mSwapChainExtent.width = chosen_extent.width;

        mScissor = {{0, 0}, {mSwapChainExtent.width, mSwapChainExtent.height}};
    }

    void VulkanRHI::ClearSwapchain()
    {
        for (auto imageview : mSwapChainImageViews)
        {
            vkDestroyImageView(mDevice, ((VulkanImageView*)imageview)->GetResource(), NULL);
        }
        vkDestroySwapchainKHR(mDevice, mSwapChain, NULL); // also swapchain images
    }

    void VulkanRHI::DestroyDefaultSampler(RHIDefaultSamplerType type)
    {
        switch (type)
        {
        case MiniEngine::Default_Sampler_Linear:
            VulkanUtil::DestroyLinearSampler(mDevice);
            delete(mLinearSampler);
            break;
        case MiniEngine::Default_Sampler_Nearest:
            VulkanUtil::DestroyNearestSampler(mDevice);
            delete(mNearestSampler);
            break;
        default:
            break;
        }
    }

    void VulkanRHI::DestroyMipmappedSampler()
    {
        VulkanUtil::DestroyMipmappedSampler(mDevice);

        for (auto sampler : mMipSamplerMap)
        {
            delete sampler.second;
        }
        mMipSamplerMap.clear();
    }

    void VulkanRHI::DestroyShaderModule(RHIShader* shaderModule)
    {
        vkDestroyShaderModule(mDevice, ((VulkanShader*)shaderModule)->GetResource(), nullptr);

        delete(shaderModule);
    }

    void VulkanRHI::DestroySemaphore(RHISemaphore* semaphore)
    {
        vkDestroySemaphore(mDevice, ((VulkanSemaphore*)semaphore)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroySampler(RHISampler* sampler)
    {
        vkDestroySampler(mDevice, ((VulkanSampler*)sampler)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyInstance(RHIInstance* instance)
    {
        vkDestroyInstance(((VulkanInstance*)instance)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyImageView(RHIImageView* imageView)
    {
        vkDestroyImageView(mDevice, ((VulkanImageView*)imageView)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyImage(RHIImage* image)
    {
        vkDestroyImage(mDevice, ((VulkanImage*)image)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyFrameBuffer(RHIFrameBuffer* framebuffer)
    {
        vkDestroyFramebuffer(mDevice, ((VulkanFrameBuffer*)framebuffer)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyFence(RHIFence* fence)
    {
        vkDestroyFence(mDevice, ((VulkanFence*)fence)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyDevice()
    {
        vkDestroyDevice(mDevice, nullptr);
    }

    void VulkanRHI::DestroyCommandPool(RHICommandPool* commandPool)
    {
        vkDestroyCommandPool(mDevice, ((VulkanCommandPool*)commandPool)->GetResource(), nullptr);
    }

    void VulkanRHI::DestroyBuffer(RHIBuffer* &buffer)
    {
        vkDestroyBuffer(mDevice, ((VulkanBuffer*)buffer)->GetResource(), nullptr);
        RHI_DELETE_PTR(buffer);
    }

    void VulkanRHI::FreeCommandBuffers(RHICommandPool* commandPool, uint32_t commandBufferCount, RHICommandBuffer* pCommandBuffers)
    {
        VkCommandBuffer vk_command_buffer = ((VulkanCommandBuffer*)pCommandBuffers)->GetResource();
        vkFreeCommandBuffers(mDevice, ((VulkanCommandPool*)commandPool)->GetResource(), commandBufferCount, &vk_command_buffer);
    }

    void VulkanRHI::FreeMemory(RHIDeviceMemory* &memory)
    {
        vkFreeMemory(mDevice, ((VulkanDeviceMemory*)memory)->GetResource(), nullptr);
        RHI_DELETE_PTR(memory);
    }

    bool VulkanRHI::MapMemory(RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size, RHIMemoryMapFlags flags, void** ppData)
    {
        VkResult result = vkMapMemory(mDevice, ((VulkanDeviceMemory*)memory)->GetResource(), offset, size, (VkMemoryMapFlags)flags, ppData);

        if (result == VK_SUCCESS)
        {
            return true;
        }
        else
        {
            LOG_ERROR("vkMapMemory failed!");
            return false;
        }
    }

    void VulkanRHI::UnmapMemory(RHIDeviceMemory* memory)
    {
        vkUnmapMemory(mDevice, ((VulkanDeviceMemory*)memory)->GetResource());
    }

    void VulkanRHI::InvalidateMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = ((VulkanDeviceMemory*)memory)->GetResource();
        mappedRange.offset = offset;
        mappedRange.size = size;
        vkInvalidateMappedMemoryRanges(mDevice, 1, &mappedRange);
    }

    void VulkanRHI::FlushMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {
        VkMappedMemoryRange mappedRange{};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = ((VulkanDeviceMemory*)memory)->GetResource();
        mappedRange.offset = offset;
        mappedRange.size = size;
        vkFlushMappedMemoryRanges(mDevice, 1, &mappedRange);
    }

    RHISemaphore* &VulkanRHI::GetTextureCopySemaphore(uint32_t index)
    {
        return mImageAvailableForTextureCopySemaphores[index];
    }

    void VulkanRHI::RecreateSwapChain()
    {
        int width  = 0;
        int height = 0;
        glfwGetFramebufferSize(mWindow, &width, &height);
        while (width == 0 || height == 0) // minimized 0,0, pause for now
        {
            glfwGetFramebufferSize(mWindow, &width, &height);
            glfwWaitEvents();
        }

        VkResult res_wait_for_fences =
            pfnVkWaitForFences(mDevice, mkMaxFramesInFlight, mIsFrameInFlightFences, VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS != res_wait_for_fences)
        {
            LOG_ERROR("pfnVkWaitForFences failed");
            return;
        }

        DestroyImageView(mDepthImageView);
        vkDestroyImage(mDevice, ((VulkanImage*)mDepthImage)->GetResource(), NULL);
        vkFreeMemory(mDevice, mDepthImageMemory, NULL);

        for (auto imageview : mSwapChainImageViews)
        {
            vkDestroyImageView(mDevice, ((VulkanImageView*)imageview)->GetResource(), NULL);
        }
        vkDestroySwapchainKHR(mDevice, mSwapChain, NULL);

        CreateSwapChain();
        CreateSwapChainImageViews();
        CreateFramebufferImageAndView();
    }

    VkResult VulkanRHI::createDebugUtilsMessengerEXT(VkInstance                                instance,
                                                     const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                     const VkAllocationCallbacks*              pAllocator,
                                                     VkDebugUtilsMessengerEXT*                 pDebugMessenger)
    {
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanRHI::destroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                                  VkDebugUtilsMessengerEXT     debugMessenger,
                                                  const VkAllocationCallbacks* pAllocator)
    {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    MiniEngine::QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physicalm_device) // for device and surface
    {
        QueueFamilyIndices indices;
        uint32_t           queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalm_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalm_device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) // if support graphics command queue
            {
                indices.graphicsFamily = i;
            }

            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) // if support compute command queue
            {
                indices.computeFamily = i;
            }


            VkBool32 is_present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalm_device,
                                                 i,
                                                 mSurface,
                                                 &is_present_support); // if support surface presentation
            if (is_present_support)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }
            i++;
        }
        return indices;
    }

    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice physicalm_device)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physicalm_device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physicalm_device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(mDeviceExtensions.begin(), mDeviceExtensions.end());
        for (const auto& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice physicalm_device)
    {
        auto queue_indices           = findQueueFamilies(physicalm_device);
        bool is_extensions_supported = checkDeviceExtensionSupport(physicalm_device);
        bool is_swapchain_adequate   = false;
        if (is_extensions_supported)
        {
            SwapChainSupportDetails swapchain_support_details = querySwapChainSupport(physicalm_device);
            is_swapchain_adequate =
                !swapchain_support_details.formats.empty() && !swapchain_support_details.presentModes.empty();
        }

        VkPhysicalDeviceFeatures physicalm_device_features;
        vkGetPhysicalDeviceFeatures(physicalm_device, &physicalm_device_features);

        if (!queue_indices.isComplete() || !is_swapchain_adequate || !physicalm_device_features.samplerAnisotropy)
        {
            return false;
        }

        return true;
    }

    MiniEngine::SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice physicalm_device)
    {
        SwapChainSupportDetails details_result;

        // capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalm_device, mSurface, &details_result.capabilities);

        // formats
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalm_device, mSurface, &format_count, nullptr);
        if (format_count != 0)
        {
            details_result.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                physicalm_device, mSurface, &format_count, details_result.formats.data());
        }

        // present modes
        uint32_t presentmode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalm_device, mSurface, &presentmode_count, nullptr);
        if (presentmode_count != 0)
        {
            details_result.presentModes.resize(presentmode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalm_device, mSurface, &presentmode_count, details_result.presentModes.data());
        }

        return details_result;
    }

    VkFormat VulkanRHI::findDepthFormat()
    {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat VulkanRHI::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                            VkImageTiling                tiling,
                                            VkFormatFeatureFlags         features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        LOG_ERROR("findSupportedFormat failed");
        return VkFormat();
    }

    VkSurfaceFormatKHR VulkanRHI::chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats)
    {
        for (const auto& surface_format : available_surface_formats)
        {
            // TODO: select the VK_FORMAT_B8G8R8A8_SRGB surface format,
            // there is no need to do gamma correction in the fragment shader
            if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return surface_format;
            }
        }
        return available_surface_formats[0];
    }

    VkPresentModeKHR VulkanRHI::chooseSwapChainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes)
    {
        for (VkPresentModeKHR present_mode : available_present_modes)
        {
            if (VK_PRESENT_MODE_MAILBOX_KHR == present_mode)
            {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRHI::chooseSwapChainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(mWindow, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanRHI::PushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color)
    {
        if (mbEnableDebugUtilsLabel)
        {
            VkDebugUtilsLabelEXT label_info;
            label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label_info.pNext = nullptr;
            label_info.pLabelName = name;
            for (int i = 0; i < 4; ++i)
                label_info.color[i] = color[i];
            pfnVkCmdBeginDebugUtilsLabelEXT(((VulkanCommandBuffer*)commond_buffer)->GetResource(), &label_info);
        }
    }

    void VulkanRHI::PopEvent(RHICommandBuffer* commond_buffer)
    {
        if (mbEnableDebugUtilsLabel)
        {
            pfnVkCmdEndDebugUtilsLabelEXT(((VulkanCommandBuffer*)commond_buffer)->GetResource());
        }
    }
    // bool VulkanRHI::isPointLightShadowEnabled(){ return mbEnablePointLightShadow; }

    RHICommandBuffer* VulkanRHI::GetCurrentCommandBuffer() const
    {
        return mCurrentCommandBuffer;
    }
    RHICommandBuffer* const* VulkanRHI::GetCommandBufferList() const
    {
        return mCommandBuffers;
    }
    RHICommandPool* VulkanRHI::GetCommandPool() const
    {
        return mRHICommandPool;
    }
    RHIDescriptorPool* VulkanRHI::GetDescriptorPool() const
    {
        return mDescPool;
    }
    RHIFence* const* VulkanRHI::GetFenceList() const
    {
        return mRHIIsFrameInFlightFences;
    }
    QueueFamilyIndices VulkanRHI::GetQueueFamilyIndices() const
    {
        return mQueueIndices;
    }
    RHIQueue* VulkanRHI::GetGraphicsQueue() const
    {
        return mGraphicsQueue;
    }
    RHIQueue* VulkanRHI::GetComputeQueue() const
    {
        return mComputeQueue;
    }
    RHISwapChainDesc VulkanRHI::GetSwapChainInfo()
    {
        RHISwapChainDesc desc;
        desc.imageFormat = mSwapChainImageFormat;
        desc.extent = mSwapChainExtent;
        desc.viewport = &mViewport;
        desc.scissor = &mScissor;
        desc.imageViews = mSwapChainImageViews;
        return desc;
    }
    RHIDepthImageDesc VulkanRHI::GetDepthImageInfo() const
    {
        RHIDepthImageDesc desc;
        desc.depthImageFormat = mDepthImageFormat;
        desc.depthImageView = mDepthImageView;
        desc.depthImage = mDepthImage;
        return desc;
    }
    uint8_t VulkanRHI::GetMaxFramesInFlight() const
    {
        return mkMaxFramesInFlight;
    }
    uint8_t VulkanRHI::GetCurrentFrameIndex() const
    {
        return mCurrentFrameIndex;
    }
    void VulkanRHI::SetCurrentFrameIndex(uint8_t index)
    {
        mCurrentFrameIndex = index;
    }
}