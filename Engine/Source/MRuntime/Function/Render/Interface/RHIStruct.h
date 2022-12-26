#pragma once

#include <optional>
#include <cstdint>
#include <vector>

#include "Vulkan/vulkan_core.h"

namespace MiniEngine
{
    class RHIQueue {};

    class RHIImageView {};

    struct RHIViewport
    {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    struct RHIExtent2D
    {
        uint32_t width;
        uint32_t height;
    };

    struct QueueFamilyIndices
    {   // 使用std::optional用于检索家族是否存在
        std::optional<uint32_t> graphicsFamily;     // 绘制图像队列家族
        std::optional<uint32_t> presentFamily;      // 显示图像队列家族

        bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    // 保存查询到的交换链支持性详细信息
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;          // 基本的表面兼容性（交换链支持的最小/最大图像数量、最小/最大图像宽高）
        std::vector<VkSurfaceFormatKHR> formats;        // 表面格式（像素格式、色彩空间）
        std::vector<VkPresentModeKHR> presentModes;     // 可用的显示模式
    };

}