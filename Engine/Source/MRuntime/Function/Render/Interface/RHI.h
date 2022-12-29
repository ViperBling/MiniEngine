#pragma once

#define GLFW_INCLUDE_VULKAN

#include <memory>

#include "Function/Render/Interface/RHIStruct.h"
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
        virtual void CreateSwapChain()                                                      = 0;
        virtual void CreateSwapChainImageViews()                                            = 0;
        virtual RHIShader* CreateShaderModule(const std::vector<unsigned char>& shaderCode) = 0;
        virtual bool CreateGraphicsPipeline(
            RHIPipelineCache* pipelineCache,
            uint32_t createInfoCnt,
            const RHIGraphicsPipelineCreateInfo* pCreateInfo,
            RHIPipeline*& pPipelines)                                                       = 0;
        virtual bool CreatePiplineLayout(
            const RHIPipelineLayoutCreateInfo* pCreateInfo,
            RHIPipelineLayout*& pPipelineLayout)                                            = 0;

        virtual bool CreateRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) = 0;
        virtual bool CreateFrameBuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFrameBuffer*& pFramebuffer) = 0;

        virtual RHISwapChainDesc GetSwapChainInfo() = 0;
    };
}