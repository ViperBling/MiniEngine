﻿#pragma once

#include <cstring>
#include <set>
#include <vector>

#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Function/Render/Interface/RHI.h"
#include "Function/Render/Interface/RHIStruct.h"
#include "Function/Render/RenderType.h"

namespace MiniEngine
{
    class VulkanRHI final : public RHI
    {
    public:
        virtual void Initialize(RHIInitInfo initInfo) override final;

        void Extracted(VkExtent2D& chosenExtent);
        void CreateSwapChain() override;
        void CreateSwapChainImageViews() override;

    private:
        void createInstance();
        void InitializeDebugMessenger();
        void createWindowSurface();
        void initializePhysicalDevice();
        void createLogicDevice();

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
        RHIQueue* mGraphicsQueue {nullptr};                         // 图形队列句柄

        RHIFormat mSwapChainImageFormat {RHI_FORMAT_UNDEFINED};     // 交换链图片格式
        RHIExtent2D mSwapChainExtent;                               // 交换链图片范围
        std::vector<RHIImageView*> mSwapChainImageViews;            // 图像的视图：描述如何访问图像以及访问图像的哪一部分
        RHIViewport mViewport;

        QueueFamilyIndices mQueueIndices;                           // 队列家族索引

        GLFWwindow*         mWindow {nullptr};
        VkInstance          mInstance {nullptr};                    // Vulkan实体
        VkSurfaceKHR        mSurface {nullptr};                     // 窗口界面
        VkPhysicalDevice    mPhysicalDevice {nullptr};              // 显卡
        VkDevice            mDevice {nullptr};                      // 逻辑设备
        VkQueue             mPresentQueue {nullptr};

        VkSwapchainKHR          mSwapChain {nullptr};               // 交换链句柄
        std::vector<VkImage>    mSwapChainImages;                   // 交换链图像句柄

        bool bEnableValidationLayers {true};                        // 启用验证层
        bool bEnableDebugUtilsLabel {true};

        VkDebugUtilsMessengerEXT mDebugMessenger {nullptr};

        const std::vector<char const*> mValidationLayers {"VK_LAYER_KHRONOS_validation"};
        uint32_t mVulkanAPIVersion {VK_API_VERSION_1_0};

        // 交换链插件：等待着被显示到屏幕上的图像的队列
        std::vector<char const*> mDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };
}