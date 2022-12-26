#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "Function/Render/WindowSystem.h"

namespace MiniEngine
{
    struct RHIInitInfo
    {
        std::shared_ptr<WindowSystem> windowSystem;
    };

    class RHI
    {
    public:
        virtual void Initialize(RHIInitInfo initInfo) = 0;

        // allocate and create
        virtual void CreateSwapChain() = 0;
        virtual void CreateSwapChainImageViews() = 0;
    };
}