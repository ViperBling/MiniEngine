#include <algorithm>
#include <cstdint>
#include <utility>

#include "VulkanRHI.h"
#include "GLFW/glfw3.h"
#include "Core/Base/Marco.h"
#include "VulkanRHIResource.h"
#include "VulkanUtil.h"
#include "vulkan/vulkan_core.h"

namespace MiniEngine
{

    void VulkanRHI::Initialize(RHIInitInfo initInfo) {

        mWindow = initInfo.windowSystem->GetWindow();
        std::array<int, 2> windowSize = initInfo.windowSystem->GetWindowSize();

        mViewport = {0.0f, 0.0f, static_cast<float>(windowSize[0]), static_cast<float>(windowSize[1]), 0.0f, 0.0f};

        // 是否启用Debug
#ifndef NDEBUG
        bEnableValidationLayers = true;
        bEnableDebugUtilsLabel = true;
#else
        bEnableValidationLayers = false;
        bEnableDebugUtilsLabel = false;
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
        createLogicDevice();
        // 创建交换链，选择最适合的属性
        CreateSwapChain();
        // 创建交换链图像视图
        CreateSwapChainImageViews();
    }

    void VulkanRHI::Extracted(VkExtent2D &chosenExtent) {

    }

    void VulkanRHI::CreateSwapChain() {

        SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(mPhysicalDevice);

        // choose the best or fitting format
        VkSurfaceFormatKHR chosenSurfaceFormat =
            chooseSwapChainSurfaceFormatFromDetails(swapChainSupportDetails.formats);
        // choose the best or fitting present mode
        VkPresentModeKHR chosenPresentMode =
            chooseSwapChainPresentModeFromDetails(swapChainSupportDetails.presentModes);
        // choose the best or fitting extent
        VkExtent2D chosenExtent = chooseSwapChainExtentFromDetails(swapChainSupportDetails.capabilities);

        // 图像渲染队列长度：尝试比默认最小值多一来实现三重缓冲
        uint32_t imageCnt = swapChainSupportDetails.capabilities.minImageCount + 1;
        // maxImageCount为0时意味着不限制内存使用，不为0时要放置image_count过界
        if (swapChainSupportDetails.capabilities.maxImageCount > 0 &&
            imageCnt > swapChainSupportDetails.capabilities.maxImageCount)
            imageCnt = swapChainSupportDetails.capabilities.maxImageCount;

        // 创建交换链
        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface         = mSurface;
        createInfo.minImageCount   = imageCnt;
        createInfo.imageFormat     = chosenSurfaceFormat.format;
        createInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
        createInfo.imageExtent     = chosenExtent;
        // 每个图像所包含的图层的数量。除非你在开发一个立体的3D程序，它的值始终应该被设为1
        createInfo.imageArrayLayers = 1;
        // 通过“颜色附件”的方式使用图像
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {mQueueIndices.graphicsFamily.value(),
                                         mQueueIndices.presentFamily.value()};
        if (mQueueIndices.graphicsFamily != mQueueIndices.presentFamily) {

            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;       // 队列家族不同时用并发模式
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;        // 队列家族相同时用显式转移图像所有权模式
        }

        createInfo.preTransform =   swapChainSupportDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode =    chosenPresentMode;
        createInfo.clipped =        VK_TRUE;    // 对遮挡像素进行裁剪

        createInfo.oldSwapchain = VK_NULL_HANDLE;   // 交换链重新创建的设置，此处假定不会重新创建

        if(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS) {
            LOG_ERROR("Vk Create SwapChain KHR Failed!");
        }

        // 虽然已经显式的指明了image数量，但vulkan实际上可能创造更多，所以需要查询一次
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCnt, nullptr);
        mSwapChainImages.resize(imageCnt);
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCnt, mSwapChainImages.data());

        // 保存交换链中的图像所选择的格式与范围到成员变量
        mSwapChainImageFormat = static_cast<RHIFormat>(chosenSurfaceFormat.format);
        mSwapChainExtent = {chosenExtent.height, chosenExtent.width};
    }

    void VulkanRHI::CreateSwapChainImageViews() {

        mSwapChainImageViews.resize(mSwapChainImages.size());

        for (size_t i = 0; i < mSwapChainImages.size(); ++i) {

            VkImageView vkImageView;
            vkImageView = VulkanUtil::CreateImageView(
                mDevice, mSwapChainImages[i], static_cast<VkFormat>(mSwapChainImageFormat),
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1
                );
            mSwapChainImageViews[i] = new VulkanImageView();
            static_cast<VulkanImageView*>(mSwapChainImageViews[i])->SetResource(vkImageView);
        }
    }

    // 初始化Vulkan实例：设置应用名称、版本、拓展模块等
    void VulkanRHI::createInstance() {

        if (bEnableValidationLayers && !checkValidationLayersSupport()) {
            LOG_ERROR("Validation Layers Requested, But not available!");
        }
        mVulkanAPIVersion = VK_API_VERSION_1_0;

        // app info
        VkApplicationInfo appInfo {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO; // 显式指定类型
        appInfo.pApplicationName   = "miniengine_renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "MiniEngine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = mVulkanAPIVersion;

        // create info: 设置全局的扩展以及验证层
        VkInstanceCreateInfo instanceCreateInfo {};
        instanceCreateInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        auto extensions =          getRequiredExtensions();
        instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
        if (bEnableValidationLayers) {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = mValidationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            instanceCreateInfo.pNext = &debugCreateInfo;
        }
        else {
            instanceCreateInfo.enabledLayerCount = 0;
            instanceCreateInfo.pNext = nullptr;
        }

        // create vulkan_context._instance
        if (vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance) != VK_SUCCESS) {
            LOG_ERROR("Vk Create Instance Failed!");
        }
    }

    // 启动验证层从而在debug版本中发现可能存在的错误
    void VulkanRHI::InitializeDebugMessenger() {

        if (bEnableValidationLayers) {
            VkDebugUtilsMessengerCreateInfoEXT create_info;
            populateDebugMessengerCreateInfo(create_info);
            if (createDebugUtilsMessengerEXT(mInstance, &create_info, nullptr, &mDebugMessenger) != VK_SUCCESS)
                LOG_ERROR("Failed to set debug messenger!");
        }
    }

    // 链接之前的glfw，使vulkan与当前运行平台窗口系统兼容
    void VulkanRHI::createWindowSurface() {

        if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
            LOG_ERROR("glfwCreateWindowSurface failed!");
    }

    void VulkanRHI::initializePhysicalDevice() {

        uint32_t physicalDeviceCnt = 0;
        vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCnt, nullptr);
        if (physicalDeviceCnt == 0) {
            LOG_ERROR("Enumerate Physical Devices Failed!");
        }
        else {
            // find one device that matches our requirement
            // or find which is the best
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCnt);
            vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCnt, physicalDevices.data());

            std::vector<std::pair<int, VkPhysicalDevice>> rankedPhysicalDevices;
            for (const auto & device : physicalDevices) {
                VkPhysicalDeviceProperties physicalDeviceProp;
                vkGetPhysicalDeviceProperties(device, &physicalDeviceProp);

                int score = 0;
                if (physicalDeviceProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    score = 1000;
                else if (physicalDeviceProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                    score = 100;

                rankedPhysicalDevices.push_back({score, device});
            }

            std::sort(rankedPhysicalDevices.begin(), rankedPhysicalDevices.end(),
                      [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2) {
                                return p1 > p2;
            });

            for (const auto & device : rankedPhysicalDevices) {
                if (isDeviceSuitable(device.second)) {
                    mPhysicalDevice = device.second;
                    break;
                }
            }

            if (mPhysicalDevice == VK_NULL_HANDLE) {
                LOG_ERROR("Failed to Find suitable physical device!");
            }
        }
    }

    // 创建逻辑设备与准备队列，从而抽象你的物理设备为一些接口
    void VulkanRHI::createLogicDevice() {

        mQueueIndices = findQueueFamilies(mPhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos; // all queues that need to be created
        std::set<uint32_t>                   queueFamilies = {mQueueIndices.graphicsFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : queueFamilies) {

            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1; // 只需要一个队列
            // 分配优先级以影响命令缓冲执行的调度。就算只有一个队列，这个优先级也是必需的
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // physical device features
        VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

        // device create info
        VkDeviceCreateInfo deviceCreateInfo {};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pEnabledFeatures        = &physicalDeviceFeatures;
        deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(mDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = mDeviceExtensions.data();
        deviceCreateInfo.enabledLayerCount       = 0;

        if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice) != VK_SUCCESS) {
            LOG_ERROR("Failed create logic device!");
        }

        // initialize queues of this device
        VkQueue vkGraphicsQueue;
        vkGetDeviceQueue(mDevice, mQueueIndices.graphicsFamily.value(), 0, &vkGraphicsQueue);
        mGraphicsQueue = new VulkanQueue();
        static_cast<VulkanQueue*>(mGraphicsQueue)->SetResource(vkGraphicsQueue); // 绑定资源

        vkGetDeviceQueue(mDevice, mQueueIndices.presentFamily.value(), 0, &mPresentQueue);
    }

    // 检查是否所有被请求的层都可用
    bool VulkanRHI::checkValidationLayersSupport() {

        uint32_t layerCnt;
        // 获得所有可用的层数
        vkEnumerateInstanceLayerProperties(&layerCnt, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCnt);
        // 获得所有可用层属性填充到availableLayers中
        vkEnumerateInstanceLayerProperties(&layerCnt, availableLayers.data());

        for (const char* layerName : mValidationLayers) {
            // 查找所有的验证层是否可用
            bool layerFound = false;
            for (const auto& layerProp : availableLayers) {
                if (std::strcmp(layerName, layerProp.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) return false;
        }

        return RHI_SUCCESS;
    }

    // 根据启用的验证层返回我们需要的插件列表(glfw)
    std::vector<const char *> VulkanRHI::getRequiredExtensions() {

        uint32_t  glfwExtensionCnt = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCnt);

        if (bEnableValidationLayers || bEnableDebugUtilsLabel) {
            extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);     // debug消息回调函数需要这个插件
        }

        return extensions;
    }

    // Debug Callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*) {

        std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    void VulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {

        createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // 回调函数在何种严重等级下被触发
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // 过滤回调函数的消息类型
        createInfo.messageType =     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback; // debug回调函数
    }

    // 创建Debug信使扩展对象
    VkResult VulkanRHI::createDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physicalDevice) {

        QueueFamilyIndices indices;
        uint32_t queueFamilyCnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCnt, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCnt);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCnt, queueFamilies.data());

        int i = 0;
        for (const auto & queueFamily : queueFamilies) {

            // find if support graphics command queue
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // find if support surface presentation
            VkBool32 isPresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, mSurface, &isPresentSupport);
            if (isPresentSupport) {
                indices.presentFamily = i;
            }

            // 各个被需求的队列家族都拥有对应索引后退出
            if (indices.isComplete()) {
                break;  // 很有可能最后得到了同一个队列家族，但是在整个程序中，我们将它们视为两个不同的独立队列
            }
            i++;
        }

        return indices;
    }

    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {

        uint32_t extensionCnt;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCnt, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCnt);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCnt, availableExtensions.data());

        // 检查是否每个被需要的插件都已经在可用插件集中了
        std::set<std::string> required_extensions(mDeviceExtensions.begin(), mDeviceExtensions.end());
        for (const auto& extension : availableExtensions)
            required_extensions.erase(extension.extensionName);

        return required_extensions.empty();

        return false;
    }

    SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice physicalDevice) {

        SwapChainSupportDetails details;

        // capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface, &details.capabilities);

        // formats
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &format_count, nullptr);
        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &format_count, details.formats.data());
        }

        // present modes
        uint32_t presentmode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface, &presentmode_count, nullptr);
        if (presentmode_count != 0)
        {
            details.presentModes.resize(presentmode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, mSurface, &presentmode_count, details.presentModes.data());
        }

        return details;
    }

    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice physicalDevice) {

        // ①所需队列家族是否存在
        auto queueIndices = findQueueFamilies(physicalDevice);

        // ②交换链所需属性与物理设备设备的兼容性检查
        bool isExtensionsSupported = checkDeviceExtensionSupport(physicalDevice);
        bool isSwapChainAdequate   = false;

        if (isExtensionsSupported) {
            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);
            isSwapChainAdequate =
                !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
        }

        return queueIndices.isComplete() && isSwapChainAdequate;
    }

    // 选择可用交换链中最好的表面格式（色彩深度）
    VkSurfaceFormatKHR VulkanRHI::chooseSwapChainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats) {

        for (const auto & surfaceFormat : availableSurfaceFormats)
        {
            // TODO: select the VK_FORMAT_B8G8R8A8_SRGB surface format,
            // there is no need to do gamma correction in the fragment shader
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return surfaceFormat; // 最好格式：sRGB
        }
        return availableSurfaceFormats[0]; // 当找不到sRGB支持时直接返回第一个表面格式
    }

    // 选择可用交换链中最好的显示模式（在屏幕上“交换”图像的条件）
    VkPresentModeKHR VulkanRHI::chooseSwapChainPresentModeFromDetails(const std::vector<VkPresentModeKHR> &availablePresentModes) {

        for (VkPresentModeKHR presentMode : availablePresentModes) {
            if (VK_PRESENT_MODE_MAILBOX_KHR == presentMode) {
                return VK_PRESENT_MODE_MAILBOX_KHR; // 最好模式为：三缓冲（需求更多显存，但延迟相对低）
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; // 默认情况为双缓冲：队列储存图像
    }

    VkExtent2D VulkanRHI::chooseSwapChainExtentFromDetails(const VkSurfaceCapabilitiesKHR &capabilities) {

        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else // 如果当前的宽高为UINT32_MAX，意味着窗体管理器允许我们做出自己的设置，则选择在minImageExtent与maxImageExtent之间最符合窗口分辨率的分辨率
        {
            int width, height;
            glfwGetFramebufferSize(mWindow, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = std::clamp(
                actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(
                actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}