#include <algorithm>
#include <cstdint>
#include <utility>

#define GLFW_INCLUDE_VULKAN

#include "VulkanRHI.h"
#include "GLFW/glfw3.h"
#include "Core/Base/Marco.h"
#include "VulkanRHIResource.h"
#include "VulkanUtil.h"
#include "vulkan/vulkan_core.h"
#include "Function/Render/RenderType.h"

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
        createCommandPool();
        createCommandBuffers();
        createSyncPrimitive();
    }

    void VulkanRHI::PrepareContext() {
        mVkCurrentCommandBuffer = mVkCommandBuffers[mCurrentFrameIndex];
        static_cast<VulkanCommandBuffer*>(mCurrentCommandBuffer)->SetResource(mVkCurrentCommandBuffer);
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
        mSwapChainExtent = {chosenExtent.width, chosenExtent.height};
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
            LOG_ERROR("Vulkan Validation Layers Requested, But not available!");
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

        // create some function pointers that may not be available in some devices
        pfnVkBeginCommandBuffer =
            reinterpret_cast<PFN_vkBeginCommandBuffer>(vkGetDeviceProcAddr(mDevice, "vkBeginCommandBuffer"));
        pfnVkEndCommandBuffer =
            reinterpret_cast<PFN_vkEndCommandBuffer>(vkGetDeviceProcAddr(mDevice, "vkEndCommandBuffer"));
        pfnVkCmdBeginRenderPass =
            reinterpret_cast<PFN_vkCmdBeginRenderPass>(vkGetDeviceProcAddr(mDevice, "vkCmdBeginRenderPass"));
        pfnVkCmdEndRenderPass =
            reinterpret_cast<PFN_vkCmdEndRenderPass>(vkGetDeviceProcAddr(mDevice, "vkCmdEndRenderPass"));
        pfnVkCmdBindPipeline =
            reinterpret_cast<PFN_vkCmdBindPipeline>(vkGetDeviceProcAddr(mDevice, "vkCmdBindPipeline"));
        pfnVkCmdSetViewport =
            reinterpret_cast<PFN_vkCmdSetViewport>(vkGetDeviceProcAddr(mDevice, "vkCmdSetViewport"));
        pfnVkCmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(vkGetDeviceProcAddr(mDevice, "vkCmdSetScissor"));
        pfnVkWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(vkGetDeviceProcAddr(mDevice, "vkWaitForFences"));
        pfnVkResetFences = reinterpret_cast<PFN_vkResetFences>(vkGetDeviceProcAddr(mDevice, "vkResetFences"));
    }

    // 创建命令池，用于管理命令缓存的内存
    void VulkanRHI::createCommandPool() {

        // graphics command pool
        // TODO
        {
            mRHICommandPool = new VulkanCommandPool();
            VkCommandPoolCreateInfo commandPoolCreateInfo {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.pNext = nullptr;
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = mQueueIndices.graphicsFamily.value();

            for (auto & mCommandPool : mCommandPools) {
                if (vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
                    LOG_ERROR("Vulkan failed to create command pool");
                }
            }
        }
    }

    // 创建命令缓冲
    void VulkanRHI::createCommandBuffers() {

        // set the command buffer allocator information
        VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        // can be pushed to a queue to execute but can not be called from any other command
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1U;

        // bind command buffer to command pool

        // 下面的代码将会与piccolo有所出入
        for (uint32_t i = 0; i < mkMaxFramesInFlight; i++) {
            commandBufferAllocateInfo.commandPool = mCommandPools[i];
            VkCommandBuffer vkCommandBuffer;
            if (vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &vkCommandBuffer) != VK_SUCCESS) {
                LOG_ERROR("Vulkan failed to allocate command buffers!");
            }
            // command buffer resource binding
            mVkCommandBuffers[i] = vkCommandBuffer;
            mCommandBuffers[i] = new VulkanCommandBuffer();

            static_cast<VulkanCommandBuffer*>(mCommandBuffers[i])->SetResource(vkCommandBuffer);
        }
    }

    // 创造同步元：信号量与栅栏
    // semaphore : signal an image is ready for rendering // ready for presentation
    // (m_vulkan_context._swapchain_images --> semaphores, fences)
    void VulkanRHI::createSyncPrimitive() {

        // sem: thread will wait for another thread signal specific sem (it makes it > 0);
        VkSemaphoreCreateInfo semaphoreCreateInfo {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // thread will stop until the work(be fenced) has done
        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // the fence is initialized as signaled to allow the first frame to be renderded
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < mkMaxFramesInFlight; i++)
        {
            if (vkCreateSemaphore(
                mDevice, &semaphoreCreateInfo, nullptr, &mImageAvailableForRenderSemaphore[i]) !=
                VK_SUCCESS ||
                vkCreateSemaphore(
                    mDevice, &semaphoreCreateInfo, nullptr, &mImageFinishedForPresentationSemaphore[i]) !=
                VK_SUCCESS ||
                vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mIsFrameInFlightFences[i]) != VK_SUCCESS)
                LOG_ERROR("Vulkan failed to  create semaphore");
        }
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

    RHIShader *VulkanRHI::CreateShaderModule(const std::vector<unsigned char> &shaderCode) {

        RHIShader* shader = new VulkanShader();
        VkShaderModule vkShader = VulkanUtil::CreateShaderModule(mDevice, shaderCode);
        static_cast<VulkanShader*>(shader)->SetResource(vkShader);

        return shader;
    }

    bool VulkanRHI::CreateGraphicsPipeline(
        RHIPipelineCache *pipelineCache, uint32_t createInfoCnt,
        const RHIGraphicsPipelineCreateInfo *pCreateInfo, RHIPipeline *&pPipelines) {

        // TODO: implement
        // int pipeline_shader_stage_create_info_size = pCreateInfo->stageCount;
        // //
        // 要使用着色器，我们需要通过VkPipelineShaderStageCreateInfo结构体把它们分配到图形渲染管线上的某一阶段，作为管线创建过程的一部分
        // std::vector<VkPipelineShaderStageCreateInfo> vk_pipeline_shader_stage_create_info_list(
        //     pipeline_shader_stage_create_info_size);

        // convert shader
        int piplineShaderStageCreateInfoSize = pCreateInfo->stageCount;
        std::vector<VkPipelineShaderStageCreateInfo> vkPipelineShaderStageCreateInfoList(piplineShaderStageCreateInfoSize);

        for (int i = 0; i < piplineShaderStageCreateInfoSize; i++) {
            const auto & rhiPipelineShaderStageCreateInfoElement = pCreateInfo->pStages[i];
            auto& vkPipelineShaderStageCreateInfoElement = vkPipelineShaderStageCreateInfoList[i];

            vkPipelineShaderStageCreateInfoElement.sType =
                static_cast<VkStructureType>(rhiPipelineShaderStageCreateInfoElement.sType);
            vkPipelineShaderStageCreateInfoElement.stage =
                static_cast<VkShaderStageFlagBits>(rhiPipelineShaderStageCreateInfoElement.stage);
            vkPipelineShaderStageCreateInfoElement.module =
                static_cast<VulkanShader*>(rhiPipelineShaderStageCreateInfoElement.module)->GetResource();
            vkPipelineShaderStageCreateInfoElement.pName = rhiPipelineShaderStageCreateInfoElement.pName;
        }

        // convert vertex input
        int vertexInputBindingDescSize = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescrList(vertexInputBindingDescSize);
        for (int i = 0; i < vertexInputBindingDescSize; ++i)
        {
            const auto& rhiVertexInputBindingDescElement =
                pCreateInfo->pVertexInputState->pVertexBindingDescriptions[i];
            auto& vkVertexInputBindingDescElement = vkVertexInputBindingDescrList[i];

            vkVertexInputBindingDescElement.binding = rhiVertexInputBindingDescElement.binding;
            vkVertexInputBindingDescElement.stride  = rhiVertexInputBindingDescElement.stride;
            vkVertexInputBindingDescElement.inputRate =
                static_cast<VkVertexInputRate>(rhiVertexInputBindingDescElement.inputRate);
        };

        // set aside
        int vertexInputAttributeDescSize = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescList(vertexInputAttributeDescSize);
        for (int i = 0; i < vertexInputAttributeDescSize; ++i) {

            const auto& rhiVertexInputAttributeDescElement =
                pCreateInfo->pVertexInputState->pVertexAttributeDescriptions[i];
            auto& vkVertexInputAttributeDescElement = vkVertexInputAttributeDescList[i];

            vkVertexInputAttributeDescElement.location = rhiVertexInputAttributeDescElement.location;
            vkVertexInputAttributeDescElement.binding = rhiVertexInputAttributeDescElement.binding;
            vkVertexInputAttributeDescElement.format = static_cast<VkFormat>(rhiVertexInputAttributeDescElement.format);
            vkVertexInputAttributeDescElement.offset = rhiVertexInputAttributeDescElement.offset;
        };

        // set aside
        VkPipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo {};
        vkPipelineVertexInputStateCreateInfo.sType = static_cast<VkStructureType>(pCreateInfo->pVertexInputState->sType);
        vkPipelineVertexInputStateCreateInfo.pNext = pCreateInfo->pVertexInputState->pNext;
        vkPipelineVertexInputStateCreateInfo.flags = static_cast<VkPipelineVertexInputStateCreateFlags>(pCreateInfo->pVertexInputState->flags);
        vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = pCreateInfo->pVertexInputState->vertexBindingDescriptionCount;
        vkPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vkVertexInputBindingDescrList.data();
        vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = pCreateInfo->pVertexInputState->vertexAttributeDescriptionCount;
        vkPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vkVertexInputAttributeDescList.data();

        VkPipelineInputAssemblyStateCreateInfo vkPipelineInputAssemblyStateCreateInfo {};
        vkPipelineInputAssemblyStateCreateInfo.sType = static_cast<VkStructureType>(pCreateInfo->pInputAssemblyState->sType);
        vkPipelineInputAssemblyStateCreateInfo.pNext = pCreateInfo->pInputAssemblyState->pNext;
        vkPipelineInputAssemblyStateCreateInfo.flags = static_cast<VkPipelineInputAssemblyStateCreateFlags>(pCreateInfo->pInputAssemblyState->flags);
        vkPipelineInputAssemblyStateCreateInfo.topology = static_cast<VkPrimitiveTopology>(pCreateInfo->pInputAssemblyState->topology);
        vkPipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = static_cast<VkBool32>(pCreateInfo->pInputAssemblyState->primitiveRestartEnable);

        // viewport
        int                     viewportSize = pCreateInfo->pViewportState->viewportCount;
        std::vector<VkViewport> vkViewportList(viewportSize);
        for (int i = 0; i < viewportSize; ++i) {
            const auto& rhiViewportElement = pCreateInfo->pViewportState->pViewports[i];
            auto&       vkViewportElement  = vkViewportList[i];

            vkViewportElement.x        = rhiViewportElement.x;
            vkViewportElement.y        = rhiViewportElement.y;
            vkViewportElement.width    = rhiViewportElement.width;
            vkViewportElement.height   = rhiViewportElement.height;
            vkViewportElement.minDepth = rhiViewportElement.minDepth;
            vkViewportElement.maxDepth = rhiViewportElement.maxDepth;
        };

        // scissor
        int                   rect2DSize = pCreateInfo->pViewportState->scissorCount;
        std::vector<VkRect2D> vkRect2DList(rect2DSize);
        for (int i = 0; i < rect2DSize; ++i)
        {
            const auto& rhiRect2DElement = pCreateInfo->pViewportState->pScissors[i];
            auto&       vkRect2DElement  = vkRect2DList[i];

            VkOffset2D offset2d {};
            offset2d.x = rhiRect2DElement.offset.x;
            offset2d.y = rhiRect2DElement.offset.y;

            VkExtent2D extend2d {};
            extend2d.width  = rhiRect2DElement.extent.width;
            extend2d.height = rhiRect2DElement.extent.height;

            vkRect2DElement.offset = offset2d;
            vkRect2DElement.extent = extend2d;
        };

        VkPipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo {};
        vkPipelineViewportStateCreateInfo.sType =
            static_cast<VkStructureType>(pCreateInfo->pViewportState->sType);
        vkPipelineViewportStateCreateInfo.pNext =
            pCreateInfo->pViewportState->pNext;
        vkPipelineViewportStateCreateInfo.flags =
            static_cast<VkPipelineViewportStateCreateFlags>(pCreateInfo->pViewportState->flags);
        vkPipelineViewportStateCreateInfo.viewportCount = pCreateInfo->pViewportState->viewportCount;
        vkPipelineViewportStateCreateInfo.pViewports    = vkViewportList.data();
        vkPipelineViewportStateCreateInfo.scissorCount  = pCreateInfo->pViewportState->scissorCount;
        vkPipelineViewportStateCreateInfo.pScissors     = vkRect2DList.data();

        // rasterization
        VkPipelineRasterizationStateCreateInfo vkPipelineRasterizationStateCreateInfo {};
        vkPipelineRasterizationStateCreateInfo.sType =
            static_cast<VkStructureType>(pCreateInfo->pRasterizationState->sType);
        vkPipelineRasterizationStateCreateInfo.pNext =
            pCreateInfo->pRasterizationState->pNext;
        vkPipelineRasterizationStateCreateInfo.flags =
            static_cast<VkPipelineRasterizationStateCreateFlags>(pCreateInfo->pRasterizationState->flags);
        vkPipelineRasterizationStateCreateInfo.depthClampEnable =
            static_cast<VkBool32>(pCreateInfo->pRasterizationState->depthClampEnable);
        vkPipelineRasterizationStateCreateInfo.rasterizerDiscardEnable =
            static_cast<VkBool32>(pCreateInfo->pRasterizationState->rasterizerDiscardEnable);
        vkPipelineRasterizationStateCreateInfo.polygonMode =
            static_cast<VkPolygonMode>(pCreateInfo->pRasterizationState->polygonMode);
        vkPipelineRasterizationStateCreateInfo.cullMode =
            static_cast<VkCullModeFlags>(pCreateInfo->pRasterizationState->cullMode);
        vkPipelineRasterizationStateCreateInfo.frontFace =
            static_cast<VkFrontFace>(pCreateInfo->pRasterizationState->frontFace);
        vkPipelineRasterizationStateCreateInfo.depthBiasEnable =
            static_cast<VkBool32>(pCreateInfo->pRasterizationState->depthBiasEnable);
        vkPipelineRasterizationStateCreateInfo.depthBiasConstantFactor =
            pCreateInfo->pRasterizationState->depthBiasConstantFactor;
        vkPipelineRasterizationStateCreateInfo.depthBiasClamp =
            pCreateInfo->pRasterizationState->depthBiasClamp;
        vkPipelineRasterizationStateCreateInfo.depthBiasSlopeFactor =
            pCreateInfo->pRasterizationState->depthBiasSlopeFactor;
        vkPipelineRasterizationStateCreateInfo.lineWidth =
            pCreateInfo->pRasterizationState->lineWidth;

        // MSAA
        VkPipelineMultisampleStateCreateInfo vkPipelineMultisampleStateCreateInfo {};
        vkPipelineMultisampleStateCreateInfo.sType =
            static_cast<VkStructureType>(pCreateInfo->pMultisampleState->sType);
        vkPipelineMultisampleStateCreateInfo.pNext = pCreateInfo->pMultisampleState->pNext;
        vkPipelineMultisampleStateCreateInfo.flags =
            static_cast<VkPipelineMultisampleStateCreateFlags>(pCreateInfo->pMultisampleState->flags);
        vkPipelineMultisampleStateCreateInfo.rasterizationSamples =
            static_cast<VkSampleCountFlagBits>(pCreateInfo->pMultisampleState->rasterizationSamples);
        vkPipelineMultisampleStateCreateInfo.sampleShadingEnable =
            static_cast<VkBool32>(pCreateInfo->pMultisampleState->sampleShadingEnable);
        vkPipelineMultisampleStateCreateInfo.minSampleShading = pCreateInfo->pMultisampleState->minSampleShading;
        vkPipelineMultisampleStateCreateInfo.pSampleMask =
            reinterpret_cast<const RHISampleMask*>(pCreateInfo->pMultisampleState->pSampleMask);
        vkPipelineMultisampleStateCreateInfo.alphaToCoverageEnable =
            static_cast<VkBool32>(pCreateInfo->pMultisampleState->alphaToCoverageEnable);
        vkPipelineMultisampleStateCreateInfo.alphaToOneEnable =
            static_cast<VkBool32>(pCreateInfo->pMultisampleState->alphaToOneEnable);

        // color blend
        int pipelineColorBlendAttachmentStateSize = pCreateInfo->pColorBlendState->attachmentCount;
        std::vector<VkPipelineColorBlendAttachmentState> vkPipelineColorBlendAttachmentStateList(pipelineColorBlendAttachmentStateSize);
        for (int i = 0; i < pipelineColorBlendAttachmentStateSize; ++i)
        {
            const auto& rhiPipelineColorBlendAttachmentStateElement =
                pCreateInfo->pColorBlendState->pAttachments[i];
            auto& vkPipelineColorBlendAttachmentStateElement = vkPipelineColorBlendAttachmentStateList[i];

            vkPipelineColorBlendAttachmentStateElement.blendEnable =
                static_cast<VkBool32>(rhiPipelineColorBlendAttachmentStateElement.blendEnable);
            vkPipelineColorBlendAttachmentStateElement.srcColorBlendFactor =
                static_cast<VkBlendFactor>(rhiPipelineColorBlendAttachmentStateElement.srcColorBlendFactor);
            vkPipelineColorBlendAttachmentStateElement.dstColorBlendFactor =
                static_cast<VkBlendFactor>(rhiPipelineColorBlendAttachmentStateElement.dstColorBlendFactor);
            vkPipelineColorBlendAttachmentStateElement.colorBlendOp =
                static_cast<VkBlendOp>(rhiPipelineColorBlendAttachmentStateElement.colorBlendOp);
            vkPipelineColorBlendAttachmentStateElement.srcAlphaBlendFactor =
                static_cast<VkBlendFactor>(rhiPipelineColorBlendAttachmentStateElement.srcAlphaBlendFactor);
            vkPipelineColorBlendAttachmentStateElement.dstAlphaBlendFactor =
                static_cast<VkBlendFactor>(rhiPipelineColorBlendAttachmentStateElement.dstAlphaBlendFactor);
            vkPipelineColorBlendAttachmentStateElement.alphaBlendOp =
                static_cast<VkBlendOp>(rhiPipelineColorBlendAttachmentStateElement.alphaBlendOp);
            vkPipelineColorBlendAttachmentStateElement.colorWriteMask =
                static_cast<VkColorComponentFlags>(rhiPipelineColorBlendAttachmentStateElement.colorWriteMask);
        };

        VkPipelineColorBlendStateCreateInfo vkPipelineColorBlendStateCreateInfo {};
        vkPipelineColorBlendStateCreateInfo.sType =
            static_cast<VkStructureType>(pCreateInfo->pColorBlendState->sType);
        vkPipelineColorBlendStateCreateInfo.pNext         = pCreateInfo->pColorBlendState->pNext;
        vkPipelineColorBlendStateCreateInfo.flags         = pCreateInfo->pColorBlendState->flags;
        vkPipelineColorBlendStateCreateInfo.logicOpEnable = pCreateInfo->pColorBlendState->logicOpEnable;
        vkPipelineColorBlendStateCreateInfo.logicOp =
            static_cast<VkLogicOp>(pCreateInfo->pColorBlendState->logicOp);
        vkPipelineColorBlendStateCreateInfo.attachmentCount = pCreateInfo->pColorBlendState->attachmentCount;
        vkPipelineColorBlendStateCreateInfo.pAttachments = vkPipelineColorBlendAttachmentStateList.data();
        for (int i = 0; i < 4; ++i) {
            vkPipelineColorBlendStateCreateInfo.blendConstants[i] =
                pCreateInfo->pColorBlendState->blendConstants[i];
        };

        // dynamic
        int                         dynamicStateSize = pCreateInfo->pDynamicState->dynamicStateCount;
        std::vector<VkDynamicState> vkDynamicStateList(dynamicStateSize);
        for (int i = 0; i < dynamicStateSize; ++i)
        {
            const auto& rhiDynamicStateElement = pCreateInfo->pDynamicState->pDynamicStates[i];
            auto&       vkDynamicStateElement  = vkDynamicStateList[i];

            vkDynamicStateElement = static_cast<VkDynamicState>(rhiDynamicStateElement);
        };

        VkPipelineDynamicStateCreateInfo vkPipelineDynamicStateCreateInfo {};
        vkPipelineDynamicStateCreateInfo.sType = static_cast<VkStructureType>(pCreateInfo->pDynamicState->sType);
        vkPipelineDynamicStateCreateInfo.pNext = pCreateInfo->pDynamicState->pNext;
        vkPipelineDynamicStateCreateInfo.flags =
            static_cast<VkPipelineDynamicStateCreateFlags>(pCreateInfo->pDynamicState->flags);
        vkPipelineDynamicStateCreateInfo.dynamicStateCount = pCreateInfo->pDynamicState->dynamicStateCount;
        vkPipelineDynamicStateCreateInfo.pDynamicStates    = vkDynamicStateList.data();

        VkGraphicsPipelineCreateInfo createInfo {};
        createInfo.sType               = static_cast<VkStructureType>(pCreateInfo->sType);
        createInfo.pNext               = pCreateInfo->pNext;
        createInfo.flags               = static_cast<VkPipelineCreateFlags>(pCreateInfo->flags);
        createInfo.stageCount          = pCreateInfo->stageCount;
        createInfo.pStages             = vkPipelineShaderStageCreateInfoList.data();
        createInfo.pVertexInputState   = &vkPipelineVertexInputStateCreateInfo;
        createInfo.pInputAssemblyState = &vkPipelineInputAssemblyStateCreateInfo;
//         createInfo.pTessellationState  = vk_pipeline_tessellation_state_create_info_ptr;
        createInfo.pViewportState      = &vkPipelineViewportStateCreateInfo;
        createInfo.pRasterizationState = &vkPipelineRasterizationStateCreateInfo;
        createInfo.pMultisampleState   = &vkPipelineMultisampleStateCreateInfo;
        createInfo.pColorBlendState    = &vkPipelineColorBlendStateCreateInfo;
        // create_info.pDepthStencilState  = &vk_pipeline_depth_stencil_state_create_info;
        createInfo.pDynamicState = &vkPipelineDynamicStateCreateInfo;
        createInfo.layout        = static_cast<VulkanPipelineLayout*>(pCreateInfo->layout)->GetResource();
        createInfo.renderPass    = static_cast<VulkanRenderPass*>(pCreateInfo->renderPass)->GetResource();
        createInfo.subpass       = pCreateInfo->subpass;
        if (pCreateInfo->basePipelineHandle != nullptr) {
            createInfo.basePipelineHandle =
                static_cast<VulkanPipeline*>(pCreateInfo->basePipelineHandle)->GetResource();
        }
        else {
            createInfo.basePipelineHandle = VK_NULL_HANDLE;
        }
        createInfo.basePipelineIndex = pCreateInfo->basePipelineIndex;

        pPipelines = new VulkanPipeline();
        VkPipeline      vkPipelines;
        VkPipelineCache vkPipelineCache = VK_NULL_HANDLE;
        if (pipelineCache != nullptr) {
            vkPipelineCache = static_cast<VulkanPipelineCache*>(pipelineCache)->GetResource();
        }
        VkResult result = vkCreateGraphicsPipelines(
            mDevice, vkPipelineCache, createInfoCnt, &createInfo, nullptr, &vkPipelines);
        static_cast<VulkanPipeline*>(pPipelines)->SetResource(vkPipelines);

        if (result != VK_SUCCESS) {
            LOG_ERROR("Vulkan failed to create GraphicsPipelines!");
            return false;
        }

        return RHI_SUCCESS;
    }

    bool VulkanRHI::CreatePiplineLayout(
        const RHIPipelineLayoutCreateInfo *pCreateInfo,
        RHIPipelineLayout *&pPipelineLayout) {

        // TODO
        VkPipelineLayoutCreateInfo createInfo {};
        createInfo.sType = static_cast<VkStructureType>(pCreateInfo->sType);
        createInfo.pNext          = pCreateInfo->pNext;
        createInfo.flags          = static_cast<VkPipelineLayoutCreateFlags>(pCreateInfo->flags);
        createInfo.setLayoutCount = pCreateInfo->setLayoutCount;
        createInfo.pSetLayouts    = nullptr; // TODO: layout descriptor

        pPipelineLayout = new VulkanPipelineLayout();
        VkPipelineLayout vkPipelineLayout;
        VkResult result = vkCreatePipelineLayout(mDevice, &createInfo, nullptr, &vkPipelineLayout);
        static_cast<VulkanPipelineLayout*>(pPipelineLayout)->SetResource(vkPipelineLayout);

        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create Vulkan pipeline layout!");
            return false;
        }

        return RHI_SUCCESS;
    }

    bool VulkanRHI::CreateRenderPass(const RHIRenderPassCreateInfo *pCreateInfo, RHIRenderPass *&pRenderPass) {

        // attachment convert
        std::vector<VkAttachmentDescription> vkAttachments(pCreateInfo->attachmentCount);
        for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
            const auto & rhiDesc = pCreateInfo->pAttachments[i];
            auto & vkDesc = vkAttachments[i];

            vkDesc.format = static_cast<VkFormat>((rhiDesc).format);
            vkDesc.samples        = static_cast<VkSampleCountFlagBits>((rhiDesc).samples);
            vkDesc.loadOp         = static_cast<VkAttachmentLoadOp>((rhiDesc).loadOp);
            vkDesc.storeOp        = static_cast<VkAttachmentStoreOp>((rhiDesc).storeOp);
            vkDesc.stencilLoadOp  = static_cast<VkAttachmentLoadOp>((rhiDesc).stencilLoadOp);
            vkDesc.stencilStoreOp = static_cast<VkAttachmentStoreOp>((rhiDesc).stencilStoreOp);
            vkDesc.initialLayout  = static_cast<VkImageLayout>((rhiDesc).initialLayout);
            vkDesc.finalLayout    = static_cast<VkImageLayout>((rhiDesc).finalLayout);
        }
        // subpass convert
        int totalAttachmentRef = 0;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
            const auto & rhiDesc = pCreateInfo->pSubpasses[i];
            totalAttachmentRef += rhiDesc.colorAttachmentCount;
        }
        std::vector<VkSubpassDescription> vkSubpassDesc(pCreateInfo->subpassCount);
        std::vector<VkAttachmentReference> vkAttachmentReference(totalAttachmentRef);

        int currentAttachmentRef = 0;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
            const auto & rhiDesc = pCreateInfo->pSubpasses[i];
            auto & vkDesc = vkSubpassDesc[i];

            vkDesc.pipelineBindPoint = static_cast<VkPipelineBindPoint>((rhiDesc).pipelineBindPoint);
            vkDesc.colorAttachmentCount = (rhiDesc).colorAttachmentCount;
            vkDesc.pColorAttachments    = &vkAttachmentReference[currentAttachmentRef];

            for (uint32_t i = 0; i < (rhiDesc).colorAttachmentCount; ++i)
            {
                const auto& rhiAttachmentRefenceColor = (rhiDesc).pColorAttachments[i];
                auto&       vkAttachmentRefenceColor  = vkAttachmentReference[currentAttachmentRef];

                vkAttachmentRefenceColor.attachment = rhiAttachmentRefenceColor.attachment;
                vkAttachmentRefenceColor.layout = static_cast<VkImageLayout>(rhiAttachmentRefenceColor.layout);

                currentAttachmentRef += 1;
            };
        }

        if (currentAttachmentRef != totalAttachmentRef)
        {
            LOG_ERROR("currentAttachmentRefence != totalAttachmentRefenrence");
            return false;
        }

        // specify the subpass dependency
        std::vector<VkSubpassDependency> vkSubpassDepandecy(pCreateInfo->dependencyCount);
        for (int i = 0; i < pCreateInfo->dependencyCount; ++i)
        {
            const auto& rhiDesc = pCreateInfo->pDependencies[i];
            auto&       vkDesc  = vkSubpassDepandecy[i];

            vkDesc.srcSubpass = rhiDesc.srcSubpass; // which will be depended on
            vkDesc.dstSubpass = rhiDesc.dstSubpass; // which has dependency
            // in which stage will to depend on happen
            vkDesc.srcStageMask = static_cast<VkPipelineStageFlags>((rhiDesc).srcStageMask);
            vkDesc.dstStageMask = static_cast<VkPipelineStageFlags>((rhiDesc).dstStageMask);
            // which operation needs dependency
            vkDesc.srcAccessMask   = static_cast<VkAccessFlags>((rhiDesc).srcAccessMask);
            vkDesc.dstAccessMask   = static_cast<VkAccessFlags>((rhiDesc).dstAccessMask);
            vkDesc.dependencyFlags = static_cast<VkDependencyFlags>((rhiDesc).dependencyFlags);
        };

        // render pass convert
        VkRenderPassCreateInfo createInfo {};
        createInfo.sType           = static_cast<VkStructureType>(pCreateInfo->sType);
        createInfo.attachmentCount = pCreateInfo->attachmentCount;
        createInfo.pAttachments    = vkAttachments.data();
        createInfo.subpassCount    = pCreateInfo->subpassCount;
        createInfo.pSubpasses      = vkSubpassDesc.data();
        createInfo.dependencyCount = pCreateInfo->dependencyCount;
        createInfo.pDependencies   = vkSubpassDepandecy.data();

        pRenderPass = new VulkanRenderPass();
        VkRenderPass vkRenderPass;
        VkResult     result = vkCreateRenderPass(mDevice, &createInfo, nullptr, &vkRenderPass);
        static_cast<VulkanRenderPass*>(pRenderPass)->SetResource(vkRenderPass);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan failed to create RenderPass!");
            return false;
        }
        return RHI_SUCCESS;
    }

    bool VulkanRHI::CreateFrameBuffer(const RHIFramebufferCreateInfo *pCreateInfo, RHIFrameBuffer *&pFrameBuffer) {

        // image view
        int imageViewSize = pCreateInfo->attachmentCount;
        std::vector<VkImageView> vkImageViewList(imageViewSize);
        for (int i = 0; i < imageViewSize; ++i)
        {
            const auto& rhiImageViewElement = pCreateInfo->pAttachments[i];
            auto&       vkImageViewElement  = vkImageViewList[i];

            vkImageViewElement = static_cast<VulkanImageView*>(rhiImageViewElement)->GetResource();
        }

        // frame buffer
        VkFramebufferCreateInfo createInfo {};
        createInfo.sType           = static_cast<VkStructureType>(pCreateInfo->sType);
        createInfo.pNext           = pCreateInfo->pNext;
        createInfo.flags           = static_cast<VkFramebufferCreateFlags>(pCreateInfo->flags);
        createInfo.renderPass      = static_cast<VulkanRenderPass*>(pCreateInfo->renderPass)->GetResource();
        createInfo.attachmentCount = pCreateInfo->attachmentCount;
        createInfo.pAttachments    = vkImageViewList.data();
        createInfo.width           = pCreateInfo->width;
        createInfo.height          = pCreateInfo->height;
        createInfo.layers          = pCreateInfo->layers;

        pFrameBuffer = new VulkanFrameBuffer();
        VkFramebuffer vkFrameBuffer;
        VkResult      result = vkCreateFramebuffer(mDevice, &createInfo, nullptr, &vkFrameBuffer);
        static_cast<VulkanFrameBuffer*>(pFrameBuffer)->SetResource(vkFrameBuffer);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan failed to create Framebuffer!");
            return false;
        }
        return RHI_SUCCESS;
    }

    void
    VulkanRHI::CmdBeginRenderPassPFN(
        RHICommandBuffer *commandBuffer,
        const RHIRenderPassBeginInfo *pRenderPassBegin,
        RHISubpassContents contents) {

        // TODO
        VkOffset2D offset2D {};
        offset2D.x = pRenderPassBegin->renderArea.offset.x;
        offset2D.y = pRenderPassBegin->renderArea.offset.y;

        VkExtent2D extent2D {};
        extent2D.width = pRenderPassBegin->renderArea.extent.width;
        extent2D.height = pRenderPassBegin->renderArea.extent.height;

        VkRect2D rect2D {};
        rect2D.offset = offset2D;
        rect2D.extent = extent2D;

        // clear_values
        int clearValueSize = pRenderPassBegin->clearValueCount;
        std::vector<VkClearValue> vkClearValueList(clearValueSize);
        for (int i = 0; i < clearValueSize; ++i)
        {
            const auto& rhiClearValueElement = pRenderPassBegin->pClearValues[i];
            auto&       vkClearValueElement  = vkClearValueList[i];

            VkClearColorValue vkClearColorValue;
            vkClearColorValue.float32[0] = rhiClearValueElement.color.float32[0];
            vkClearColorValue.float32[1] = rhiClearValueElement.color.float32[1];
            vkClearColorValue.float32[2] = rhiClearValueElement.color.float32[2];
            vkClearColorValue.float32[3] = rhiClearValueElement.color.float32[3];
            vkClearColorValue.int32[0]   = rhiClearValueElement.color.int32[0];
            vkClearColorValue.int32[1]   = rhiClearValueElement.color.int32[1];
            vkClearColorValue.int32[2]   = rhiClearValueElement.color.int32[2];
            vkClearColorValue.int32[3]   = rhiClearValueElement.color.int32[3];
            vkClearColorValue.uint32[0]  = rhiClearValueElement.color.uint32[0];
            vkClearColorValue.uint32[1]  = rhiClearValueElement.color.uint32[1];
            vkClearColorValue.uint32[2]  = rhiClearValueElement.color.uint32[2];
            vkClearColorValue.uint32[3]  = rhiClearValueElement.color.uint32[3];

            VkClearDepthStencilValue vkClearDepthStencilValue;
            vkClearDepthStencilValue.depth   = rhiClearValueElement.depthStencil.depth;
            vkClearDepthStencilValue.stencil = rhiClearValueElement.depthStencil.stencil;

            vkClearValueElement.color        = vkClearColorValue;
            vkClearValueElement.depthStencil = vkClearDepthStencilValue;
        };

        VkRenderPassBeginInfo vkRenderPassBeginInfo {};
        vkRenderPassBeginInfo.sType = static_cast<VkStructureType>(pRenderPassBegin->sType);
        vkRenderPassBeginInfo.pNext = pRenderPassBegin->pNext;
        vkRenderPassBeginInfo.renderPass =
            static_cast<VulkanRenderPass*>(pRenderPassBegin->renderPass)->GetResource();
        vkRenderPassBeginInfo.framebuffer =
            static_cast<VulkanFrameBuffer*>(pRenderPassBegin->framebuffer)->GetResource();
        vkRenderPassBeginInfo.renderArea      = rect2D; // where the shader effects
        vkRenderPassBeginInfo.clearValueCount = pRenderPassBegin->clearValueCount;
        vkRenderPassBeginInfo.pClearValues    = vkClearValueList.data();

        return pfnVkCmdBeginRenderPass(
            static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource(),
            &vkRenderPassBeginInfo,
            static_cast<VkSubpassContents>(contents)
            );
    }

    void VulkanRHI::CmdBindPipelinePFN(
        RHICommandBuffer *commandBuffer,
        RHIPipelineBindPoint pipelineBindPoint,
        RHIPipeline *pipeline) {

        return pfnVkCmdBindPipeline(
            static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource(),
            static_cast<VkPipelineBindPoint>(pipelineBindPoint),
            static_cast<VulkanPipeline*>(pipeline)->GetResource()
            );
    }

    void VulkanRHI::CmdDraw(
        RHICommandBuffer *commandBuffer,
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance) {

        vkCmdDraw(static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource(),
                  vertexCount,
                  instanceCount,
                  firstVertex,
                  firstInstance);
    }

    void VulkanRHI::CmdEndRenderPassPFN(RHICommandBuffer *commandBuffer) {

        return pfnVkCmdEndRenderPass(
            static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource());
    }

    void VulkanRHI::CmdSetViewportPFN(
        RHICommandBuffer *commandBuffer,
        uint32_t firstViewport,
        uint32_t viewportCount,
        const RHIViewport *pViewports) {

        // viewport
        int viewportSize = viewportCount;
        std::vector<VkViewport> vkViewportList(viewportSize);
        for (int i = 0; i < viewportSize; i++) {

            const auto& rhiViewportElement = pViewports[i];
            auto&       vkViewportElement  = vkViewportList[i];

            vkViewportElement.x        = rhiViewportElement.x;
            vkViewportElement.y        = rhiViewportElement.y;
            vkViewportElement.width    = rhiViewportElement.width;
            vkViewportElement.height   = rhiViewportElement.height;
            vkViewportElement.minDepth = rhiViewportElement.minDepth;
            vkViewportElement.maxDepth = rhiViewportElement.maxDepth;
        }
        return pfnVkCmdSetViewport(
            static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource(),
            firstViewport,
            viewportCount,
            vkViewportList.data()
            );
    }

    void VulkanRHI::CmdSetScissorPFN(
        RHICommandBuffer *commandBuffer,
        uint32_t firstScissor,
        uint32_t scissorCount,
        const RHIRect2D *pScissors) {

        // rect_2d
        int rect2DSize = scissorCount;
        std::vector<VkRect2D> vkRect2DList(rect2DSize);
        for (int i = 0; i < rect2DSize; ++i)
        {
            const auto& rhiRect2DElement = pScissors[i];
            auto&       vkRect2DElement  = vkRect2DList[i];

            VkOffset2D offset2D {};
            offset2D.x = rhiRect2DElement.offset.x;
            offset2D.y = rhiRect2DElement.offset.y;

            VkExtent2D extent2D {};
            extent2D.width  = rhiRect2DElement.extent.width;
            extent2D.height = rhiRect2DElement.extent.height;

            vkRect2DElement.offset = VkOffset2D(offset2D);
            vkRect2DElement.extent = VkExtent2D(extent2D);
        };

        return pfnVkCmdSetScissor(
            static_cast<VulkanCommandBuffer*>(commandBuffer)->GetResource(),
            firstScissor,
            scissorCount,
            vkRect2DList.data());
    }

    void VulkanRHI::WaitForFences() {

        if (pfnVkWaitForFences(
            mDevice, 1,
            &mIsFrameInFlightFences[mCurrentFrameIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {

            LOG_ERROR("Vulkan failed to synchronize fences!");
        }
    }

    RHISwapChainDesc VulkanRHI::GetSwapChainInfo() {

        RHISwapChainDesc desc;
        desc.imageFormat = mSwapChainImageFormat;
        desc.extent = mSwapChainExtent;
        desc.viewport = &mViewport;
        desc.scissor = &mScissor;
        desc.imageViews = mSwapChainImageViews;

        return desc;
    }

    RHICommandBuffer* VulkanRHI::GetCurrentCommandBuffer() const {
        return mCurrentCommandBuffer;
    }

    bool VulkanRHI::PrepareBeforePass() {

        // acquire next image from swapchain
        VkResult acquireImageResult = vkAcquireNextImageKHR(
            mDevice,
            mSwapChain,
            UINT64_MAX,
            mImageAvailableForRenderSemaphore[mCurrentFrameIndex],
            VK_NULL_HANDLE,
            &mCurrentSwapChainImageIndex);

        // begin command buffer
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // allow to resubmit command buffer even if it is already in waiting list
        commandBufferBeginInfo.flags            = RHI_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        if (pfnVkBeginCommandBuffer(mVkCommandBuffers[mCurrentFrameIndex], &commandBufferBeginInfo) != VK_SUCCESS) {
            LOG_ERROR("Vulkan failed to begin recording command buffer!");
            return false;
        }
        return RHI_SUCCESS;
    }

    void VulkanRHI::SubmitRendering() {

        // end command buffer
        if (pfnVkEndCommandBuffer(mVkCommandBuffers[mCurrentFrameIndex]) != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan EndCommandBuffer failed!");
            return;
        }

        // submit command buffer
        // signal(semaphore[])
        VkSemaphore signalSemaphores[] = {mImageFinishedForPresentationSemaphore[mCurrentFrameIndex]};
        // wait for color attachment output
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo         submitInfo   = {};
        submitInfo.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = &mImageAvailableForRenderSemaphore[mCurrentFrameIndex];
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &mVkCommandBuffers[mCurrentFrameIndex];
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = signalSemaphores;

        if (pfnVkResetFences(mDevice, 1, &mIsFrameInFlightFences[mCurrentFrameIndex]) != VK_SUCCESS) {

            // reset fence state(unsignaled)
            LOG_ERROR("Vulkan ResetFences failed!");
            return;
        }

        // submit info(command buffer) to graphics queue family
        if (vkQueueSubmit(
            static_cast<VulkanQueue*>(mGraphicsQueue)->GetResource(),
            1,
            &submitInfo,
            // submit finished, allow another render
            mIsFrameInFlightFences[mCurrentFrameIndex]) != VK_SUCCESS) {

            LOG_ERROR("Vulkan QueueSubmit failed!");
            return;
        }

        // present swapchain
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &mImageFinishedForPresentationSemaphore[mCurrentFrameIndex];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &mSwapChain;
        presentInfo.pImageIndices      = &mCurrentSwapChainImageIndex;

        if (vkQueuePresentKHR(mPresentQueue, &presentInfo) != VK_SUCCESS)
        {
            LOG_ERROR("Vulkan QueuePresent failed!");
            return;
        }

        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mkMaxFramesInFlight;
    }




}